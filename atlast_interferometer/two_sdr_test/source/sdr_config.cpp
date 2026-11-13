#include "sdrplay_api.h"
#include <iostream>


//These are the configuration functions to be called
//this is just ot get the api and lock it
int get_sdr_api(sdrplay_api_DeviceT devs[], int max_devs,  int chosenIdx,
        sdrplay_api_DeviceT* &chosenDevice, sdrplay_api_ErrT &err){
    //this function initialize the sdr api.. if fails it returns 1
    float ver=0;
    unsigned int ndev=0;
    // Open API
    if ((err = sdrplay_api_Open()) != sdrplay_api_Success){
        std::cout << "sdrplay_api_Open failed" << sdrplay_api_GetErrorString(err) << "\n";
        return 1;
    }
    // Enable debug logging output
    if ((err = sdrplay_api_DebugEnable(NULL, sdrplay_api_DbgLvl_Verbose)) != sdrplay_api_Success){
        std::cout << "sdrplay_api_DebugEnable failed" << sdrplay_api_GetErrorString(err) << "\n";
        return 1;
    }
    // Check API versions match
    if ((err = sdrplay_api_ApiVersion(&ver)) != sdrplay_api_Success){
        std::cout << "sdrplay_api_ApiVersion failed " << sdrplay_api_GetErrorString(err)<< "\n";
        return 1;
    }
    if (ver != SDRPLAY_API_VERSION){
        std::cout << "API version don't match (local:"<< SDRPLAY_API_VERSION <<", dll:" << ver <<"\n";
        sdrplay_api_Close();
        return 1;
    }

    std::cout << "locking api\n";
    sdrplay_api_LockDeviceApi();
    if((err = sdrplay_api_GetDevices(devs, &ndev, max_devs)) != sdrplay_api_Success){
        std::cout << "sdrplay_api_GetDevices failed"<<sdrplay_api_GetErrorString(err) << "\n";
        sdrplay_api_UnlockDeviceApi();
        sdrplay_api_Close();
        return 1;
    }

    //std::cout << "MaxDevs="<< sizeof(devs) / sizeof(sdrplay_api_DeviceT) <<" NumDevs="<<ndev <<"\n";
    if (ndev == 0){
        std::cout << "No devices found to connect\n";
        sdrplay_api_UnlockDeviceApi();
        sdrplay_api_Close();
        return 1;
    }
    //alwuys take the id 0
    //chosenIdx = 0;
    std::cout << "chosenDevice=" << chosenIdx << "\n";
    chosenDevice = &devs[chosenIdx];        //not sure of this!
    
    std::cout << "check if the selected device is the RSPduo.. ";
    if (chosenDevice->hwVer != SDRPLAY_RSPduo_ID){
        printf("Device is not RSPDuo\n");
        sdrplay_api_UnlockDeviceApi();
        sdrplay_api_Close();
        return 1;
    }
    return 0;
}


int free_api(){
    std::cout << "unlock api..";
    sdrplay_api_UnlockDeviceApi();
    std::cout << "done\n";
    std::cout << "Close api..";
    sdrplay_api_Close();
    std::cout << "done\n";
    return 0;
}

int configureChoseDevice(sdrplay_api_DeviceT* &chosenDevice, 
        sdrplay_api_DeviceParamsT* &deviceParams,
        sdrplay_api_RxChannelParamsT* &chParamsA, 
        sdrplay_api_RxChannelParamsT* &chParamsB, 
        hyperparameters &params,
        sdrplay_api_ErrT &err
        ){
    //check hw version
    if (chosenDevice->hwVer != SDRPLAY_RSPduo_ID){
        std::cout << "Device is not RSPDuo\n";
        //sdrplay_api_UnlockDeviceApi();
        sdrplay_api_Close();
        return 1;
    }
    chosenDevice->tuner = sdrplay_api_Tuner_Both;
    chosenDevice->rspDuoMode = sdrplay_api_RspDuoMode_Dual_Tuner;
    chosenDevice->rspDuoSampleFreq = params.fsamp;

    std::cout << "Rspduo mode :"<< chosenDevice->rspDuoMode << "\ttuner: "
        << chosenDevice->tuner << "\trspDuoSampleFreq: " << chosenDevice->rspDuoSampleFreq
        << std::endl;
    //try to open the device
    if ((err = sdrplay_api_SelectDevice(chosenDevice)) != sdrplay_api_Success) {
        std::cout << "sdrplay_api_SelectDevice failed\n"<< sdrplay_api_GetErrorString(err) ;
        sdrplay_api_UnlockDeviceApi();
        sdrplay_api_Close();
        return 1;
    }
    //if everything ok unlock api
    sdrplay_api_UnlockDeviceApi();
    // Retrieve device parameters so they can be changed if wanted
    if ((err = sdrplay_api_GetDeviceParams(chosenDevice->dev, &deviceParams)) !=sdrplay_api_Success){
        std::cout << "sdrplay_api_GetDeviceParams failed\n";
        sdrplay_api_Close();
        return 1;
    }
    //check if indeed we got the values on the deviceParams
    if(deviceParams == NULL){
        std::cout << "getDeviceParams returned null pointer \n";
        sdrplay_api_Close();
        return 1;
    }
    deviceParams->devParams->fsFreq.fsHz = params.fsamp;
    chParamsA = deviceParams->rxChannelA;
    chParamsB = deviceParams->rxChannelB;
    //chParamsA->tunerParams.rfFreq.rfHz = 220000000.0;
    //chParamsB->tunerParams.rfFreq.rfHz = 220000000.0;
    chParamsA->tunerParams.rfFreq.rfHz = params.flo;
    chParamsB->tunerParams.rfFreq.rfHz = params.flo;
    chParamsA->tunerParams.ifType = sdrplay_api_IF_2_048;
    chParamsB->tunerParams.ifType = sdrplay_api_IF_2_048;

    chParamsA->tunerParams.bwType = sdrplay_api_BW_1_536;
    chParamsB->tunerParams.bwType = sdrplay_api_BW_1_536;
    chParamsA->tunerParams.loMode = sdrplay_api_LO_Auto;
    chParamsB->tunerParams.loMode = sdrplay_api_LO_Auto;
    chParamsA->tunerParams.gain.gRdB = params.gain;
    chParamsB->tunerParams.gain.gRdB = params.gain;
    // Disable AGC
    chParamsA->ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;
    chParamsB->ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;
    //again choose  dual tuner
    chosenDevice->tuner = sdrplay_api_Tuner_Both;
    chosenDevice->rspDuoMode = sdrplay_api_RspDuoMode_Dual_Tuner;
    chosenDevice->rspDuoSampleFreq = params.fsamp;


    return 0;

}

int intialize_device(sdrplay_api_DeviceT* &chosenDevice, 
        sdrplay_api_CallbackFnsT &cbFns,
        sdrplay_api_ErrT &err){

    if((err = sdrplay_api_Init(chosenDevice->dev, &cbFns, NULL)) != sdrplay_api_Success){
        std::cout << "sdrplay_api_init failed..." << sdrplay_api_GetErrorString(err) << "\n";
        if(err == sdrplay_api_StartPending){
            std::cout << "start pending.. trying again..\n";
            while(1){
                sleep(1000);
                //the master should be initialized at the eventcallback... 
                //but I havent see thats needed
                if(masterInitialized){
                    if( ((err = sdrplay_api_Init(chosenDevice->dev, &cbFns, NULL)) != sdrplay_api_Success)){
                            std::cout <<"sdrplay_api_Init failed.." << sdrplay_api_GetErrorString(err) <<"\n";
                            break;
                }
            }
            std::cout << "Waiting the master to initialize...\n";
        }

    }
        //get other types of errors..
        else{
            sdrplay_api_ErrorInfoT *errInfo = sdrplay_api_GetLastError(NULL);
            if(errInfo != NULL){
                std::cout << "Error in " << errInfo->file << ": "<<errInfo->function <<
                    "(): line " <<errInfo->line << ": "<< errInfo->message << "\n";
                sdrplay_api_Close();
                exit(0);
            }
        }

    }
    return 0;
}

