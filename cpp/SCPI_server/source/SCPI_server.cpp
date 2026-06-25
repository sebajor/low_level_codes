#include "../includes/SCPI_server.h"
#include <iostream>
#include <string>
#include <charconv>
#include <sstream>
#include <iomanip>


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



int SCPI_server::fitsWritterFormatter(std::string& out_msg, double amp, double phase,
        std::chrono::system_clock::time_point stamp){
    //The format of the fitswritter message is:
    //encoding      char[4]: IEEE
    //data format   char[4]: F+3blank
    //packetlen     int: 
    //backend id    char[8]: HOLOBE
    //timestamp     char[28]: YYYY-mm-ddTHH:MM:SS.ssss[TAI‖GPS‖UTC]+1 blank
    //integTime     int: In us
    //phasenum      int: 1 for us
    //numBeSections int: 1
    //blocking      int: 10 (above 1 assumes integration time steps)
    //***upto here we have 64 bytes
    //data: BeSec<int>+numChannel<int>2, amp<float>, phase<float>

    //data should be 2 size (amp and phase)
   
    out_msg = "";
    out_msg += fitsWritterMsgParams.encoding;
    out_msg += fitsWritterMsgParams.beFormat;
    
    out_msg.append(reinterpret_cast<char*>(&fitsWritterMsgParams.pkt_len), 
            sizeof(fitsWritterMsgParams.pkt_len));
    
    out_msg += fitsWritterMsgParams.beName;
    SCPI_server::fitsWritterTimestamp(out_msg, stamp, fitsWritterMsgParams.timeSystem);
    
    out_msg.append(reinterpret_cast<char*>(&fitsWritterMsgParams.integTime_us),
            sizeof(fitsWritterMsgParams.integTime_us));

    out_msg.append(reinterpret_cast<char*>(&fitsWritterMsgParams.phaseNum),
            sizeof(fitsWritterMsgParams.phaseNum));

    out_msg.append(reinterpret_cast<char*>(&fitsWritterMsgParams.numBeSec),
            sizeof(fitsWritterMsgParams.numBeSec));

    out_msg.append(reinterpret_cast<char*>(&fitsWritterMsgParams.blockFactor),
            sizeof(fitsWritterMsgParams.blockFactor));

    //Here we start with the data part
    out_msg.append(reinterpret_cast<char*>(&fitsWritterMsgParams.numBeSec),
           sizeof(fitsWritterMsgParams.numBeSec));  //Be section (in priniple we should do like a for loop
                                                    //
    out_msg.append(reinterpret_cast<char*>(&fitsWritterMsgParams.numChannels),
            sizeof(fitsWritterMsgParams.numChannels));

    float amp_f =  static_cast<float>(amp);
    float phase_f = static_cast<float>(phase);

    out_msg.append(reinterpret_cast<char*>(&amp_f),
        sizeof(amp_f));

    out_msg.append(reinterpret_cast<char*>(&phase_f),
        sizeof(phase_f));
    return 0;
}



int SCPI_server::fitsWritterTimestamp(std::string &out_msg, 
        std::chrono::system_clock::time_point stamp,
        std::string_view timeSystem
        ){
    using namespace std::chrono;

    //auto now = system_clock::now();

    auto ms =
        duration_cast<milliseconds>(
            stamp.time_since_epoch()) % 1000;

    auto tt = system_clock::to_time_t(stamp);

    std::tm tm = *gmtime(&tt);

    std::ostringstream oss;

    oss << std::put_time(
                &tm,
                "%Y-%m-%dT%H:%M:%S")
        << '.'
        << std::setw(4)
        << std::setfill('0')
        << ms.count();
    out_msg +=oss.str();
    out_msg += timeSystem;
    return 0;
}

int SCPI_server::fitsWritterTest(std::string_view msg, std::string& out_msg){
    std::cout << "asdasd";
    auto now = std::chrono::system_clock::now();
    double amp =  1.2423;
    double phase=  1.2423;
    SCPI_server::fitsWritterFormatter(out_msg, amp, phase, now);
    std::cout << out_msg << "\n";
    return 0;
}
