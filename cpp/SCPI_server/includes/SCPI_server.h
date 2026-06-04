#include "../../UdpUtils/includes/UdpSocket.h"
#include <vector>
#include <iostream>
#include <utility>
#include <string>

class SCPI_server {
    private:
        int sock {-1};
        sockaddr_in client_addr;
        int internal_value {0};
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

        
        //commands and the corresponden methods
        //
        using cmdHandler = int(SCPI_server::*)(std::string_view, std::string&);
        std::vector<std::pair<std::string, cmdHandler>> cmds {
            {"SCPI_SERVER:GET_VALUE", &SCPI_server::get_value},
            {"SCPI_SERVER:SET_VALUE", &SCPI_server::set_value},
            {"SCPI_SERVER:HELP", &SCPI_server::help},
        };
};
