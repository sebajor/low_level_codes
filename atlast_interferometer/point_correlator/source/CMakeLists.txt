cmake_minimum_required(VERSION 3.25)
project(phase_correlator)

include_directories(../includes
)

add_executable(${PROJECT_NAME} 
    main.cpp
)


# ---- FFTW ----
find_library(FFTW_LIB fftw3)

if(NOT FFTW_LIB)
    message(FATAL_ERROR "fftw3 library not found")
else()
    message(STATUS "fftw3 found: ${FFTW_LIB}")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${FFTW_LIB})

# ---- SDRplay ----
find_library(SDRPLAY_LIB sdrplay_api)

if(NOT SDRPLAY_LIB)
    message(FATAL_ERROR "sdrplay_api library not found")
else()
    message(STATUS "sdrplay_api found: ${SDRPLAY_LIB}")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${SDRPLAY_LIB})




