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


//signal handler
std::atomic<bool> g_stop(false);
//ping-pong buffers hyperparamaters
constexpr int vectorsize=2048;
constexpr int N_BUFFERS=2;


//SDR0
std::atomic<int> adc0_0_ready=0, adc1_0_ready=0;
std::atomic<int> adc0_0_counter=0, adc1_0_counter=0;
std::atomic<bool> chA0Started=0, chB0Started=0;
std::atomic<uint64_t> firstSampleA0=0, firstSampleB0=0;

std::vector<short> adc0_0_re[N_BUFFERS];
std::vector<short> adc0_0_im[N_BUFFERS];
std::vector<short> adc1_0_re[N_BUFFERS];
std::vector<short> adc1_0_im[N_BUFFERS];



//this one should be common for both sdr
std::atomic<int> write_buffer_index =0;
std::atomic<int> nsamples_global=32;



//bullshit
std::atomic<int> masterInitialized=0;
std::atomic<int> slaveInitialized=0;


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
int writedata_adc(int iteration, int sdr_number,
        std::vector<short> &adc0_re, std::vector<short> &adc0_im, 
        std::vector<short> &adc1_re, std::vector<short> &adc1_im,
        int adc0_first_sample, int adc1_first_sample, int numsamples
        ){
    std::ofstream file("sdr"+std::to_string(sdr_number) +"data_iter"+
            std::to_string(iteration)+".txt");
    if(!file.is_open())
        return 1;
    file << adc0_first_sample<< " ";
    file << adc1_first_sample<< "\n";
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


//callbacks SDR0
void StreamA0Callback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, 
        unsigned int numSamples, unsigned int reset, void *cbContext){
    if(reset)
        std::cout << "StreamAcallback: numSamples=" << numSamples << 
            " firstSampleNum="<< params->firstSampleNum<<"\n";
    for(int i=0; i<numSamples;++i){
        if(!chA0Started.load(std::memory_order_acquire)){
            firstSampleA0.store(params->firstSampleNum + i, std::memory_order_relaxed);
            adc0_0_counter.store(0, std::memory_order_relaxed);
            chA0Started.store(true, std::memory_order_release);
        }
        if(adc0_0_counter!=nsamples_global){
            adc0_0_re[write_buffer_index][adc0_0_counter] = xi[i];
            adc0_0_im[write_buffer_index][adc0_0_counter] = xq[i];
            adc0_0_counter++;
            if(adc0_0_counter==nsamples_global){
                adc0_0_ready = 1;
            }
        }
    }
}

void StreamBCallback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, 
        unsigned int numSamples, unsigned int reset, void *cbContext){
    if(reset)
        std::cout << "StreamBcallback: numSamples=" << numSamples <<
            " firstSampleNum="<< params->firstSampleNum<<"\n";
    //this is just a test
    for(int i=0; i<numSamples;++i){
        if(!chB0Started.load(std::memory_order_acquire)){
            firstSampleB0.store(params->firstSampleNum + i, std::memory_order_relaxed);
            adc1_0_counter.store(0, std::memory_order_relaxed);
            chB0Started.store(true, std::memory_order_release);
        }
        //if(!adc1_ready){
        if(adc1_0_counter != nsamples_global){
            adc1_0_re[write_buffer_index][adc1_0_counter] = xi[i];
            adc1_0_im[write_buffer_index][adc1_0_counter] = xq[i];
            adc1_0_counter++;
            if(adc1_0_counter==nsamples_global){
                adc1_0_ready = 1;

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
    sdrplay_api_DeviceT *chosenDevice0 = NULL;
    sdrplay_api_DeviceParamsT *deviceParams0 = NULL;
    sdrplay_api_CallbackFnsT cbFns0;
    sdrplay_api_RxChannelParamsT *chParams0,*chParamsA0,*chParamsB0;
    
    //no idea if i need just one of this..
    sdrplay_api_ErrT err;

    if(get_sdr_api(devs, max_devs, 0,
                chosenDevice0, err))
        exit(0);
    if(configureChoseDevice(chosenDevice0, deviceParams0,
                chParamsA0, chParamsB0,
                params, err))
        exit(0);
    //Assign callback functions to be passed to sdrplay_api_Init()
    cbFns0.StreamACbFn = StreamA0Callback;
    cbFns0.StreamBCbFn = StreamB0Callback;
    cbFns0.EventCbFn = EventCallback;


    //
    //
    //

    //set the global numsmaples to iterate over
    nsamples_global = params.batches*params.nsamp+params.nsamp_overhead;

    //initialize the buffers
    for(int i=0; i<N_BUFFERS;++i){
        adc0_0_re[i].resize(params.batches*params.nsamp+params.nsamp_overhead);
        adc0_0_im[i].resize(params.batches*params.nsamp+params.nsamp_overhead);
        adc1_0_re[i].resize(params.batches*params.nsamp+params.nsamp_overhead);
        adc1_0_im[i].resize(params.batches*params.nsamp+params.nsamp_overhead);
    }

    //generate stop signal handler
    std::signal(SIGINT, signal_handler);   // Ctrl+C
    std::signal(SIGTERM, signal_handler);  // kill
                                           //
    std::cout << "got the api\n";
    std::cout << "configuring the receivers...";
    intialize_device(chosenDevice0, cbFns0, err);
    std::cout << "done\n";

    
    int current_read=0;
    int local_batch_num=0;
    int next_write = 0;
    int iteration=0;
    while(!g_stop){
        //here should be all the adcs!
        if(adc0_0_ready.load() && adc1_0_ready.load()){
            current_read =  write_buffer_index.load(std::memory_order_acquire); 
            //TODO: this only work if I just have 2 buffers!
            next_write = 1-current_read;
            write_buffer_index.store(next_write, std::memory_order_release);

            //restart the data collecting
            adc0_0_ready.store(0);
            chA0Started.store(0);
            adc1_0_ready.store(0);
            chB0Started.store(0);
            if(writedata_adc(iteration, 0,
                adc0_0_re[current_read], adc0_0_im[current_read], 
                adc1_0_re[current_read], adc1_0_im[current_read],
                firstSampleA0, firstSampleB0, numsamples)
                    std::cout << "error writing the data out\n";
            iteration ++;
        }
    }

    std::cout << "Release the api...";
    sdrplay_api_ReleaseDevice(chosenDevice0);
    std::cout << "done\n";
    free_api();
    return 0;
}
