#include "sdrplay_api.h"

int get_sdr_api(sdrplay_api_DeviceT devs[], int max_devs,  int chosenIdx,
        sdrplay_api_DeviceT* &chosenDevice, sdrplay_api_ErrT &err);

int free_api();

int configureChoseDevice(sdrplay_api_DeviceT* &chosenDevice, 
        sdrplay_api_DeviceParamsT* &deviceParams,
        sdrplay_api_RxChannelParamsT* &chParamsA, 
        sdrplay_api_RxChannelParamsT* &chParamsB, 
        hyperparameters &params,
        sdrplay_api_ErrT &err
        );

int intialize_device(sdrplay_api_DeviceT* &chosenDevice, 
        sdrplay_api_CallbackFnsT &cbFns,
        sdrplay_api_ErrT &err);
