#include "../../UdpUtils/includes/UdpSocket.h"
#include <vector>
#include <iostream>
#include <utility>
#include <string>
#include <chrono>

/*
struct fitsWritterParams {
    char encoding[4] = "IEEE";  //or I am UTC?  4
    char beFormat[4] = "F   ";  //              4 
    int32_t pkt_len = 80;
    char beName[8] = "HOLOBE "; //              8
    char timeSystem[4] ="TAI "; //              4 (blank at the end!)
    //char[28] stamp = "";    //YYYY-mm-ddTHH:MM:SS.ssss[TAI‖GPS‖UTC]+1 blank
    int32_t integTime_us = 100;
    int32_t phaseNum = 1;
    int32_t numBeSec = 2;   //amp, phase
    int32_t blockFactor = 3;//if larger than 1 assumes equidistant steps
    int32_t numChannels = 2;
};
*/

struct fitsWritterParams {
    std::string_view encoding = "IEEE";  //or I am UTC?  4
    std::string_view beFormat = "F   ";  //              4 
    int32_t pkt_len = 80;
    std::string_view beName = "HOLOBE  "; //              8
    std::string_view timeSystem ="TAI "; //              4 (blank at the end!)
    //std::string_view[28] stamp = "";    //YYYY-mm-ddTHH:MM:SS.ssss[TAI‖GPS‖UTC]+1 blank
    int32_t integTime_us = 100;
    int32_t phaseNum = 1;
    int32_t numBeSec = 2;   //amp, phase
    int32_t blockFactor = 3;//if larger than 1 assumes equidistant steps
    int32_t numChannels = 2;
};

class SCPI_server {
    private:
        int sock {-1};
        sockaddr_in client_addr;
        int internal_value {0};
        fitsWritterParams fitsWritterMsgParams;
    public:
        SCPI_server(std::string_view ip, int port, int timeout=-1, 
                int sendsize=1024, int recvsize=1024, int reuse_addr=1);
        ~SCPI_server();
        
        char buffer[2048] {0};
        std::string ans {0};
        int parse_recv_message(const char* recv_buff, int recv_bytes, std::string& out_msg);
        
        int get_value(std::string_view msg, std::string& out_msg);
        int set_value(std::string_view msg, std::string& out_msg);
        int help(std::string_view msg, std::string& out_msg);

        int check_message();
        int answer_request(std::string &out_msg);


        int fitsWritterFormatter(std::string& out_msg, double amp, double phase, 
            std::chrono::system_clock::time_point stamp);

        int fitsWritterTimestamp(std::string &out_msg, 
            std::chrono::system_clock::time_point stamp,
            std::string_view timeSystem
        );

        int fitsWritterTest(std::string_view msg, std::string& out_msg);
        
        //commands and the corresponden methods
        //
        using cmdHandler = int(SCPI_server::*)(std::string_view, std::string&);
        std::vector<std::pair<std::string, cmdHandler>> cmds {
            {"SCPI_SERVER:GET_VALUE", &SCPI_server::get_value},
            {"SCPI_SERVER:SET_VALUE", &SCPI_server::set_value},
            {"SCPI_SERVER:HELP", &SCPI_server::help},
            {"SCPI_SERVER:TEST", &SCPI_server::fitsWritterTest},
        };
};
