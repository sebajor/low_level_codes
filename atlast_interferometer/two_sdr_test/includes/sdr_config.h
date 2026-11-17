#include "sdrplay_api.h"


int get_sdr_api(sdrplay_api_DeviceT* devs, int max_devs,sdrplay_api_ErrT &err);


int select_device(sdrplay_api_DeviceT* devs, int chosenIdx,
            sdrplay_api_DeviceT* &chosenDevice);


int free_api();

int configureChoseDevice(sdrplay_api_DeviceT* &chosenDevice, 
        sdrplay_api_DeviceParamsT* &deviceParams,
        sdrplay_api_RxChannelParamsT* &chParamsA, 
        sdrplay_api_RxChannelParamsT* &chParamsB, 
        hyperparameters &params,
        sdrplay_api_ErrT &err,
	bool ref_out_en=false
        );

int intialize_device(sdrplay_api_DeviceT* &chosenDevice, 
        sdrplay_api_CallbackFnsT &cbFns,
        sdrplay_api_ErrT &err);

