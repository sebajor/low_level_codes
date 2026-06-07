#include "../../UdpUtils/includes/UdpSocket.h"
#include <vector>
#include <iostream>
#include <utility>
#include <string>
#include <atomic>
#include <mutex>
#include <thread>

class SCPI_server {
    private:
        int sock {-1};
        int tcp_socket{-1};
        std::string_view tcp_dest_ip {"10.0.33.133"};
        int tcp_dest_port {12334};

        sockaddr_in client_addr;
        int internal_value {0};

        std::atomic<bool> stream_running {false};
        std::atomic<bool> stream_paused {false};
        std::atomic<bool> thread_alive {false};
        std::thread worker;


    public:
        SCPI_server(std::string_view ip, int port, int timeout=-1, 
                int sendsize=1024, int recvsize=1024, int reuse_addr=1);
        ~SCPI_server();
        
        char buffer[2048] {0};
        std::string ans {0};
        int parse_recv_message(const char* recv_buff, int recv_bytes, std::string& out_msg);
        
        int get_value(std::string_view msg, std::string& out_msg);
        int set_value(std::string_view msg, std::string& out_msg);
        int start_stream(std::string_view msg, std::string& out_msg);
        int pause_stream(std::string_view msg, std::string& out_msg);
        int resume_stream(std::string_view msg, std::string& out_msg);
        int close_stream(std::string_view msg, std::string& out_msg);
        int help(std::string_view msg, std::string& out_msg);

        int setDestStreamIP(std::string_view msg, std::string& out_msg);
        int setDestStreamPort(std::string_view msg, std::string& out_msg);


        int addTimestampAnswer(std::string &out_msg);
        int check_message();
        int answer_request(std::string &out_msg);

        void workerLoop();
        

        
        //commands and the corresponden methods
        //
        using cmdHandler = int(SCPI_server::*)(std::string_view, std::string&);
        std::vector<std::pair<std::string, cmdHandler>> cmds {
            {"SCPI_SERVER:GET_VALUE", &SCPI_server::get_value},
            {"SCPI_SERVER:SET_VALUE", &SCPI_server::set_value},
            {"SCPI_SERVER:HELP", &SCPI_server::help},
            {"SCPI_SERVER:START_STREAM", &SCPI_server::start_stream },
            {"SCPI_SERVER:PAUSE_STREAM", &SCPI_server::pause_stream },
            {"SCPI_SERVER:RESUME_STREAM", &SCPI_server::resume_stream },
            {"SCPI_SERVER:CLOSE_STREAM", &SCPI_server::close_stream },
            {"SCPI_SERVER:SET_DEST_IP", &SCPI_server::setDestStreamIP},
            {"SCPI_SERVER:SET_DEST_PORT", &SCPI_server::setDestStreamPort},
        };
};

