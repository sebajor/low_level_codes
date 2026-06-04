#include "../includes/SCPI_server.h"
#include <iostream>
#include <string>
#include <charconv>


SCPI_server::SCPI_server(std::string_view host, int port, int timeout,
        int sendsize, int recvsize, int reuse_addr){
    sock = UdpSocket::socket_config(-1, sendsize, recvsize, timeout, reuse_addr);
    if(UdpSocket::bindSocket(sock, host, port))
        std::cout << "Failed to create the socket!\n";
}

SCPI_server::~SCPI_server(){
    UdpSocket::closeSocket(this->sock);
}

int SCPI_server::check_message(){
    int recv_bytes = UdpSocket::recvFrom(sock,
            buffer, 
            sizeof(buffer),
            client_addr);
    return recv_bytes;
}

int SCPI_server::answer_request(std::string &out_msg){
    int bytes_out =UdpSocket::sendTo(sock, out_msg.data(), out_msg.size(), client_addr);
    return bytes_out;
}

int SCPI_server::parse_recv_message(const char* recv_buff, int recv_bytes, std::string &out_msg){
    std::string_view input_data(buffer, static_cast<size_t>(recv_bytes));
    for(auto& pair: this->cmds){
        if(input_data.find(pair.first) != std::string_view::npos){
            std::cout << "match! "<< pair.first << "\n";
            int cmd_out = (this->*pair.second)(input_data, out_msg);

            return 1;
        }
    }
    std::cout << "no match\n";
    out_msg.assign("undefined command!\n");
    return -1;
}

//
//  These are the command that actually does something
//

int SCPI_server::get_value(std::string_view input_data, std::string &out_msg){
    out_msg = "SCPI_SERVER:GET_VALUE "+std::to_string(this->internal_value)+"\n";
    return 0;
}

int SCPI_server::set_value(std::string_view input_data, std::string &out_msg){
    size_t space = input_data.find(" ");
    if(space!= std::string_view::npos){
        std::string_view s_arg = input_data.substr(space+1);
        float arg;
        auto result = std::from_chars(s_arg.data(), s_arg.data()+s_arg.size(), arg);
        if(result.ec != std::errc{}){
            out_msg = std::string(input_data.substr(0,space))+ " ERROR!\n";
                    return 1;
        }
        internal_value = arg;
        out_msg = std::string(input_data.substr(0, space))+" Ok!\n";
    }
    return 0;
}


int SCPI_server::help(std::string_view input_data, std::string &out_msg){
    out_msg = "This is a help message\n";
    return 0;
}

