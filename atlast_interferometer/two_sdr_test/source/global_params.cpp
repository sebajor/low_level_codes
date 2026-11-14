#include "../includes/global_params.h"

//signal handler
std::atomic<bool> g_stop(false);


//SDR0
std::atomic<int> adc0_0_ready=0, adc1_0_ready=0;
std::atomic<int> adc0_0_counter=0, adc1_0_counter=0;
std::atomic<bool> chA0Started=0, chB0Started=0;
std::atomic<uint64_t> firstSampleA0=0, firstSampleB0=0;

std::vector<short> adc0_0_re[N_BUFFERS];
std::vector<short> adc0_0_im[N_BUFFERS];
std::vector<short> adc1_0_re[N_BUFFERS];
std::vector<short> adc1_0_im[N_BUFFERS];

//SDR1
std::atomic<int> adc0_1_ready=0, adc1_1_ready=0;
std::atomic<int> adc0_1_counter=0, adc1_1_counter=0;
std::atomic<bool> chA1Started=0, chB1Started=0;
std::atomic<uint64_t> firstSampleA1=0, firstSampleB1=0;

std::vector<short> adc0_1_re[N_BUFFERS];
std::vector<short> adc0_1_im[N_BUFFERS];
std::vector<short> adc1_1_re[N_BUFFERS];
std::vector<short> adc1_1_im[N_BUFFERS];

//this one should be common for both sdr
std::atomic<int> write_buffer_index =0;
std::atomic<int> nsamples_global=32;



//bullshit
std::atomic<int> masterInitialized=0;
std::atomic<int> slaveInitialized=0;

