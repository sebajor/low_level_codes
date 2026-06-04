cmake_minimum_required(VERSION 3.25)
project(tcp_server)

include_directories(../includes)

add_executable(${PROJECT_NAME} 
    test.cpp
    TcpSocket.cpp
)

