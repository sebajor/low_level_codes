#include "sdr_config.h"
#include <iostream>
#include <string>
#include "sdrplay_api.h"
#include <atomic>
#include <vector>
#include <fftw3.h>
#include <unistd.h>
#include <fstream>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <csignal>

/*  
 *  bcs of the way that the api was written it needs global variables...
 *  To control them Im using atomic variables as mutex like
 */

//signal handler
std::atomic<bool> g_stop(false);


std::atomic<int> adc0_1_ready=0, adc1_1_ready=0;
std::atomic<int> adc0_1_counter=0, adc1_1_counter=0;
std::atomic<int> write_buffer1_index=0;

std::atomic<bool> chA1Started=0, chB1Started=0;
std::atomic<uint64_t> firstSampleA1 {0}, firstSampleB1 {0};

std::atomic<int> adc0_2_ready=0, adc1_2_ready=0;
std::atomic<int> adc0_2_counter=0, adc1_2_counter=0;
std::atomic<int> write_buffer2_index=0;

std::atomic<bool> chA2Started=0, chB2Started=0;
std::atomic<uint64_t> firstSampleA2 {0}, firstSampleB2 {0};


//bullshit
std::atomic<int> masterInitialized=0;
std::atomic<int> slaveInitialized=0;


constexpr int vectorsize=2048;
constexpr int N_BUFFERS=2;

std::vector<short> adc0_1_re[N_BUFFERS];
std::vector<short> adc0_1_im[N_BUFFERS];
std::vector<short> adc1_1_re[N_BUFFERS];
std::vector<short> adc1_1_im[N_BUFFERS];

std::vector<short> adc0_2_re[N_BUFFERS];
std::vector<short> adc0_2_im[N_BUFFERS];
std::vector<short> adc1_2_re[N_BUFFERS];
std::vector<short> adc1_2_im[N_BUFFERS];

std::atomic<int> nsamples_global=32;


struct hyperparameters {
    double flo {250000000.};
    double fsamp {8000000.};        //with 8_000_000 you got 2_000_000 in reality..
    long nsamp {1024};
    double gain{40.0};
    uint64_t batches {100};
    int batch_integration {100};      //how many batches to integrate..TODO: how convert this to time!
    long nsamp_overhead {60};  //samples extras to handle missaligment.
    int window {100};           //window around the peak that you want to save, If window <0 then we save the whole bandwidth
    int ch2save {-1};           //channel to save, if <0 then use the first iteration to get the maximum
};


void signal_handler(int signum){
    g_stop.store(true, std::memory_order_relaxed);
}


int parse_args(int argc, char* argv[], hyperparameters &params){
    for(int i=1; i<argc; ++i){
        try{
            std::string arg = argv[i];
            if(arg == "-flo" && (i+1)<argc)
                params.flo= std::stod(argv[++i]);
            else if(arg == "-fsam" && (i+1)<argc)
                params.fsamp= std::stod(argv[++i]);
            else if(arg == "-nsam" && (i+1)<argc)
                params.nsamp = std::stol(argv[++i]);
            else if(arg == "-gain" && (i+1)<argc)
                params.gain = std::stod(argv[++i]);
            else if(arg == "-batches" && (i+1)<argc)
                params.batches = std::stoi(argv[++i]);
            else if(arg == "-batch_integration" && (i+1)<argc)
                params.batch_integration = std::stoi(argv[++i]);
            else if(arg == "-window" && (i+1)<argc)
                params.window= std::stoi(argv[++i]);
        }
        catch(const std::exception &e){
            std::cout << "Error in parameter "<< argv[i] << "\n";
            }
    }
    return 1;
}

//write data out to a file.. this is just plain text..
int writedata_adc(int iteration, 
        std::vector<short> &adc0_re, std::vector<short> &adc0_im, 
        std::vector<short> &adc1_re, std::vector<short> &adc1_im,
        int numsamples, int missalign
        ){
    std::ofstream file("adcdata_iter"+std::to_string(iteration)+".txt");
    if(!file.is_open())
        return 1;
    file << missalign << "\n";
    for(int i=0; i<numsamples; ++i){
        file << adc0_re[i];
        file << " ";
        file << adc0_im[i];
        file << " ";
        file << adc1_re[i];
        file << " ";
        file << adc1_im[i];
        file << "\n";
    }
    return 0;
}


int write_data_corr_masked(
        std::vector<float> &power0, std::vector<float> &power1, 
        std::vector<float> &corr_re, std::vector<float> &corr_im,
        int window, int main_peak
        ){
    auto now = std::chrono::system_clock::now();
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    //std::cout << "timestamp: "<< timestamp << "\n";


    std::ofstream file("correlation", std::ios::binary | std::ios::app);
    if(!file.is_open())
        return 1;

    if(window<0){
        file.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
        file.write(reinterpret_cast<const char*>(power0.data()),  power0.size() * sizeof(float));
        file.write(reinterpret_cast<const char*>(power1.data()),  power1.size() * sizeof(float));
        file.write(reinterpret_cast<const char*>(corr_re.data()), corr_re.size() * sizeof(float));
        file.write(reinterpret_cast<const char*>(corr_im.data()), corr_im.size() * sizeof(float));
        return 0;
    }
    else{
        file.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
        file.write(reinterpret_cast<const char*>(power0.data()+main_peak-window/2),  window * sizeof(float));
        file.write(reinterpret_cast<const char*>(power1.data()+main_peak-window/2),  window * sizeof(float));
        file.write(reinterpret_cast<const char*>(corr_re.data()+main_peak-window/2), window * sizeof(float));
        file.write(reinterpret_cast<const char*>(corr_im.data()+main_peak-window/2), window * sizeof(float));
        return 0;
    }
}



///These are the callbacks to get the data from the sdr.. note that they are
///inherently asyncronus, but lucklly the the params here have a member that
///is the firstSampleNum so we can see if the data is aligned or not
//
//
void StreamA1Callback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, 
        unsigned int numSamples, unsigned int reset, void *cbContext){
    if(reset)
        std::cout << "StreamA1callback: numSamples=" << numSamples << 
            " firstSampleNum="<< params->firstSampleNum<<"\n";
    for(int i=0; i<numSamples;++i){
        if(!chA1Started.load(std::memory_order_acquire)){
            firstSampleA1.store(params->firstSampleNum + i, std::memory_order_relaxed);
            adc0_1_counter.store(0, std::memory_order_relaxed);
            chA1Started.store(true, std::memory_order_release);
        }
        if(adc0_1_counter!=nsamples_global){
            adc0_1_re[write_buffer1_index][adc0_1_counter] = xi[i];
            adc0_1_im[write_buffer1_index][adc0_1_counter] = xq[i];
            adc0_1_counter++;
            if(adc0_1_counter==nsamples_global){
                adc0_1_ready = 1;
            }
        }
    }
}

void StreamA2Callback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, 
        unsigned int numSamples, unsigned int reset, void *cbContext){
    if(reset)
        std::cout << "StreamA2callback: numSamples=" << numSamples << 
            " firstSampleNum="<< params->firstSampleNum<<"\n";
    for(int i=0; i<numSamples;++i){
        if(!chA2Started.load(std::memory_order_acquire)){
            firstSampleA2.store(params->firstSampleNum + i, std::memory_order_relaxed);
            adc0_2_counter.store(0, std::memory_order_relaxed);
            chA2Started.store(true, std::memory_order_release);
        }
        if(adc0_2_counter!=nsamples_global){
            adc0_2_re[write_buffer2_index][adc0_2_counter] = xi[i];
            adc0_2_im[write_buffer2_index][adc0_2_counter] = xq[i];
            adc0_2_counter++;
            if(adc0_2_counter==nsamples_global){
                adc0_2_ready = 1;
            }
        }
    }
}





void StreamB1Callback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, 
        unsigned int numSamples, unsigned int reset, void *cbContext){
    if(reset)
        std::cout << "StreamBcallback: numSamples=" << numSamples <<
            " firstSampleNum="<< params->firstSampleNum<<"\n";
    //this is just a test
    for(int i=0; i<numSamples;++i){
        if(!chBS1tarted.load(std::memory_order_acquire)){
            firstSampleB1.store(params->firstSampleNum + i, std::memory_order_relaxed);
            adc1_1_counter.store(0, std::memory_order_relaxed);
            chB1Started.store(true, std::memory_order_release);
        }
        //if(!adc1_ready){
        if(adc1_1_counter != nsamples_global){
            adc1_1_re[write_buffer1_index][adc1_1_counter] = xi[i];
            adc1_1_im[write_buffer1_index][adc1_1_counter] = xq[i];
            adc1_1_counter++;
            if(adc1_1_counter==nsamples_global){
                adc1_1_ready = 1;

            }
        }
    }
}



void StreamB2Callback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, 
        unsigned int numSamples, unsigned int reset, void *cbContext){
    if(reset)
        std::cout << "StreamBcallback: numSamples=" << numSamples <<
            " firstSampleNum="<< params->firstSampleNum<<"\n";
    //this is just a test
    for(int i=0; i<numSamples;++i){
        if(!chBS2tarted.load(std::memory_order_acquire)){
            firs2SampleB1.store(params->firstSampleNum + i, std::memory_order_relaxed);
            adc1_2_counter.store(0, std::memory_order_relaxed);
            chB2Started.store(true, std::memory_order_release);
        }
        //if(!adc1_ready){
        if(adc1_2_counter != nsamples_global){
            adc1_2_re[write_buffer2_index][adc1_2_counter] = xi[i];
            adc1_2_im[write_buffer2_index][adc1_2_counter] = xq[i];
            adc1_2_counter++;
            if(adc1_2_counter==nsamples_global){
                adc1_2_ready = 1;

            }
        }
    }

}




void EventCallback(sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner,
        sdrplay_api_EventParamsT *params, void *cbContext){
}





int main(int argc, char *argv[]){
    hyperparameters params;
    parse_args(argc, argv, params);
    std::cout << "user parameters:\n\tLO: "<< params.flo << 
        "\n\tsampling rate: "<< params.fsamp <<
        "\n\tnum samples: "<< params.nsamp <<
        "\n\tgain:" << params.gain << 
        "\n\tfft batches" << params.batches <<
        "\n\tintegration batches" << params.batch_integration <<
        "\n\twindow:" << params.window << 
        std::endl;
    float integration_time = params.nsamp*params.batches*params.batch_integration/(2*1e6);
    std::cout << "With these parameters you have "<< integration_time <<"s as integration time\n";

    //declare sdrplay structs
    constexpr int max_devs=6;
    sdrplay_api_DeviceT devs[max_devs];
    sdrplay_api_DeviceT *chosenDevice1 = NULL;
    sdrplay_api_DeviceParamsT *deviceParams1 = NULL;
    sdrplay_api_CallbackFnsT cbFns1;
    sdrplay_api_RxChannelParamsT *chParams1,*chParamsA1,*chParamsB1;

    sdrplay_api_DeviceT *chosenDevice2 = NULL;
    sdrplay_api_DeviceParamsT *deviceParams2 = NULL;
    sdrplay_api_CallbackFnsT cbFns2;
    sdrplay_api_RxChannelParamsT *chParams2,*chParamsA2,*chParamsB2;

    sdrplay_api_ErrT err;
    //fft variables
    /*
    fftw_plan p0, p1;
    //fftw_complex in0[vectorsize], in1[vectorsize], out0[vectorsize], out1[vectorsize];
    fftw_complex* in0 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*params.nsamp);
    fftw_complex* in1 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*params.nsamp);
    fftw_complex* out0 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*params.nsamp);
    fftw_complex* out1 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*params.nsamp);
    */

    //start sdrplay api in the first sdr
    if(get_sdr_api(devs, max_devs, 0,
                chosenDevice1, err))
        exit(0);
    if(configureChoseDevice(chosenDevice1, deviceParams1, 
                chParamsA1, chParamsB1,
                params, err))
        exit(0);
    //Assign callback functions to be passed to sdrplay_api_Init()
    cbFns1.StreamACbFn = StreamA1Callback;
    cbFns1.StreamBCbFn = StreamB1Callback;
    cbFns1.EventCbFn = EventCallback;
   

    std::cout << "api started in the first sdr\n";

    //now we started in the second one
    if(get_sdr_api(devs, max_devs, 0,
                chosenDevice2, err))
        exit(0);
    if(configureChoseDevice(chosenDevice2, deviceParams2, 
                chParamsA2, chParamsB2,
                params, err))
        exit(0);

    //Assign callback functions to be passed to sdrplay_api_Init()
    cbFns2.StreamACbFn = StreamA2Callback;
    cbFns2.StreamBCbFn = StreamB2Callback;
    cbFns2.EventCbFn = EventCallback;
    

    //set the global numsmaples to iterate over
    nsamples_global = params.batches*params.nsamp+params.nsamp_overhead;




    //generate stop signal handler
    std::signal(SIGINT, signal_handler);   // Ctrl+C
    std::signal(SIGTERM, signal_handler);  // kill
    std::cout << "got the api\n";
    std::cout << "configuring the receivers...";
    intialize_device(chosenDevice1, cbFns1, err);
    std::cout << "done1\n";
    intialize_device(chosenDevice2, cbFns2, err);
    std::cout << "done1\n";


    /*
    //configuring FFT
    std::cout << "configuring FFTs plan\n";
    p0 = fftw_plan_dft_1d(params.nsamp, in0,out0, FFTW_FORWARD, FFTW_ESTIMATE);
    p1 = fftw_plan_dft_1d(params.nsamp, in1,out1, FFTW_FORWARD, FFTW_ESTIMATE);

    //configuring the input/output vectors
    std::vector<float> power0(vectorsize);
    std::vector<float> power1(vectorsize);
    std::vector<float> corr_re(vectorsize);
    std::vector<float> corr_im(vectorsize);

    power0.resize(params.nsamp);
    power1.resize(params.nsamp);
    corr_re.resize(params.nsamp);
    corr_im.resize(params.nsamp);
    
    for(int i=0; i< N_BUFFERS; ++i){
        adc0_re[i].resize(params.batches*params.nsamp+params.nsamp_overhead);
        adc0_im[i].resize(params.batches*params.nsamp+params.nsamp_overhead);
        adc1_re[i].resize(params.batches*params.nsamp+params.nsamp_overhead);
        adc1_im[i].resize(params.batches*params.nsamp+params.nsamp_overhead);
    }


    std::cout << "ok, ready for get some values...\n";
    int peak_location = params.ch2save;
    float max_power0 = 0;
    int max_index = 0;
    
    //finally start the while loop
    int iter_count = 0;
    int inputVal=0;
    //
    int missalign = 0;
    int current_read=0;
    int local_batch_num=0;
    int next_write = 0;
    int batch_counter=0;
    while(!g_stop){
        if(adc0_ready.load() && adc1_ready.load()){
            missalign = firstSampleA-firstSampleB;
            //std::cout << "samples ready.. missaligment: "<< missalign <<"\n";
            //std::cout << "FirstsampleA: "<< firstSampleA << " FirstSampleB:" << firstSampleB << "\n";
            current_read = write_buffer_index.load(std::memory_order_acquire);      //check that I got the old value!!
            next_write = 1-current_read;
            //CHECK!!! we need to enforce the writing of these register in this order!
            write_buffer_index.store(next_write, std::memory_order_release);
            adc0_ready.store(0);
            chAStarted.store(0);
            adc1_ready.store(0);
            chBStarted.store(0);
            //check that the missaligment is in bounds!!
            if(missalign>params.nsamp_overhead ||  missalign < -params.nsamp_overhead)
                local_batch_num= params.batches-1;
            else
                local_batch_num = params.batches;
            //in this scheme we have several batches..
            //
            //This is when the adc1 is lagged..ie missalign>0
            if(missalign>=0){
                for(int k=0; k<local_batch_num; ++k){
                    for(int i=0; i<params.nsamp; ++i){
                        in0[i][0] = static_cast<float>(adc0_re[current_read][k*params.nsamp+i]);
                        in0[i][1] = static_cast<float>(adc0_im[current_read][k*params.nsamp+i]);
                        in1[i][0] = static_cast<float>(adc1_re[current_read][k*params.nsamp+i+missalign]);
                        in1[i][1] = static_cast<float>(adc1_im[current_read][k*params.nsamp+i+missalign]);
                    }
                    //compute fft
                    fftw_execute(p0);
                    fftw_execute(p1);
                    //done, compute correaltion.. in principle to save time you can
                    //do it in the window that you are interested
                    for(int i=0; i< params.nsamp; ++i){
                        power0[i] += out0[i][0]*out0[i][0]+out0[i][1]*out0[i][1];
                        power1[i] += out1[i][0]*out1[i][0]+out1[i][1]*out1[i][1];
                        if(power0[i] > max_power0){
                            max_power0 = power0[i];
                            max_index = i;
                        }
                        //(a+ib)*(c-id) = ((ac+bd)+i(bc-ad))
                        corr_re[i] += out0[i][0]*out1[i][0]+out0[i][1]*out1[i][1];
                        corr_im[i] += out0[i][1]*out1[i][0]-out0[i][0]*out1[i][1];
                    }
                }
            }
            else{
                for(int k=0; k<local_batch_num; ++k){
                    for(int i=0; i<params.nsamp; ++i){
                        in0[i][0] = static_cast<float>(adc0_re[current_read][k*params.nsamp+i-missalign]);
                        in0[i][1] = static_cast<float>(adc0_im[current_read][k*params.nsamp+i-missalign]);
                        in1[i][0] = static_cast<float>(adc1_re[current_read][k*params.nsamp+i]);
                        in1[i][1] = static_cast<float>(adc1_im[current_read][k*params.nsamp+i]);
                    }
                    //compute fft
                    fftw_execute(p0);
                    fftw_execute(p1);
                    //done, compute correaltion.. in principle to save time you can
                    //do it in the window that you are interested only..
                    for(int i=0; i< params.nsamp; ++i){
                        power0[i] += out0[i][0]*out0[i][0]+out0[i][1]*out0[i][1];
                        power1[i] += out1[i][0]*out1[i][0]+out1[i][1]*out1[i][1];
                        if(power0[i] > max_power0){
                            max_power0 = power0[i];
                            max_index = i;
                        }
                        //(a+ib)*(c-id) = ((ac+bd)+i(bc-ad))
                        corr_re[i] += out0[i][0]*out1[i][0]+out0[i][1]*out1[i][1];
                        corr_im[i] += out0[i][1]*out1[i][0]-out0[i][0]*out1[i][1];
                    }
                }
            }
            batch_counter++;
            if(batch_counter==params.batch_integration){
                //get the timestamp
                //std::cout << "Debug: phase=" << std::atan2(corr_im[17], corr_re[17])*180/M_PI << "\n";
                //std::cout << "writing data out..";
                if(peak_location<0){
                    peak_location = max_index;
                }

                //if(write_data_corr(power0, power1, corr_re, corr_im))
                //    std::cout << "error!\n";
                //std::cout << "peak location:" << peak_location << "\n";
                if(write_data_corr_masked(power0, power1, corr_re, corr_im, 
                            params.window, peak_location))
                    std::cout << "error!\n";
                //std::cout << "done\n";
                batch_counter = 0;
                for(int i=0; i< params.nsamp; ++i){
                    power0[i] = 0;
                    power1[i] = 0;
                    corr_re[i] = 0;
                    corr_im[i] = 0;
                }
            }
        }
    }
    std::cout << "cleaning FFT..";
    fftw_destroy_plan(p0);
    fftw_destroy_plan(p1);
    fftw_cleanup();
    std::cout << "done\n";
    */
    std::cout << "Release the api...";
    sdrplay_api_ReleaseDevice(chosenDevice);
    std::cout << "done\n";
    free_api();
    return 0;
}
