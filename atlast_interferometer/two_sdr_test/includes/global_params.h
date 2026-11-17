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
#include <thread>


//signal handler
extern std::atomic<bool> g_stop;
//ping-pong buffers hyperparamaters
constexpr int vectorsize=2048;
constexpr int N_BUFFERS=2;


//SDR0
extern std::atomic<int> adc0_0_ready, adc1_0_ready;
extern std::atomic<int> adc0_0_counter, adc1_0_counter;
extern std::atomic<bool> chA0Started, chB0Started;
extern std::atomic<uint64_t> firstSampleA0, firstSampleB0;

extern std::vector<short> adc0_0_re[N_BUFFERS];
extern std::vector<short> adc0_0_im[N_BUFFERS];
extern std::vector<short> adc1_0_re[N_BUFFERS];
extern std::vector<short> adc1_0_im[N_BUFFERS];

//SDR1
extern std::atomic<int> adc0_1_ready, adc1_1_ready;
extern std::atomic<int> adc0_1_counter, adc1_1_counter;
extern std::atomic<bool> chA1Started, chB1Started;
extern std::atomic<uint64_t> firstSampleA1, firstSampleB1;

extern std::vector<short> adc0_1_re[N_BUFFERS];
extern std::vector<short> adc0_1_im[N_BUFFERS];
extern std::vector<short> adc1_1_re[N_BUFFERS];
extern std::vector<short> adc1_1_im[N_BUFFERS];

//this one should be common for both sdr
extern std::atomic<int> write_buffer_index;
extern std::atomic<int> nsamples_global;



//bullshit
extern std::atomic<int> masterInitialized;
extern std::atomic<int> slaveInitialized;


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

