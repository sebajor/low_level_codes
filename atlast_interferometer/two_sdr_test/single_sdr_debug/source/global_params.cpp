#include "../includes/global_params.h"

//signal handler
std::atomic<bool> g_stop(false);
//ping-pong buffers hyperparamaters
//constexpr int vectorsize=2048;
//constexpr int N_BUFFERS=2;

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



/*
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
*/
