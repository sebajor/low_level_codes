cmake_minimum_required(VERSION 3.25)
project(scpi_server)

include_directories(../includes
                    ../../UdpUtils/includes
)

add_executable(${PROJECT_NAME} 
    main.cpp
    SCPI_server.cpp
    ../../UdpUtils/source/UdpSocket.cpp
)


