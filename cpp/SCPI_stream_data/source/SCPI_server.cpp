#include "../includes/SCPI_server.h"
#include "../../TcpUtils/includes/TcpSocket.h"
#include <iostream>
#include <string>
#include <charconv>
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>

SCPI_server::SCPI_server(std::string_view host, int port, int timeout,
        int sendsize, int recvsize, int reuse_addr){
    sock = UdpSocket::socket_config(-1, sendsize, recvsize, timeout, reuse_addr);
    if(UdpSocket::bindSocket(sock, host, port))
        std::cout << "Failed to create the socket!\n";
}

SCPI_server::~SCPI_server(){
    UdpSocket::closeSocket(this->sock);
}

//worker function
void SCPI_server::workerLoop(){
    std::random_device seed;    //get seed
    std::mt19937 gen(seed());     //initialize the random engine
    std::uniform_int_distribution<int64_t> distr(0, 32846);   //define range

    //double aa,bb,ab_re,ab_im, stamp {0};
    int64_t data[5] {42};


    tcp_socket = TcpSocket::socket_config(tcp_socket);      //here I should also modify the default buffers and stuff...
    int conn = TcpSocket::connectSocket(tcp_socket, tcp_dest_ip, tcp_dest_port);
    if(conn == -1){
        TcpSocket::closeSocket(tcp_socket);
        thread_alive = false;
        return;
    }
    std::cout << "tcp connection output: "<< conn << "\n";
    thread_alive = true;
    std::cout << "startng thread while 1\n";
    int ret_val {0};
    //TcpSocket::sendStringData(tcp_socket, std::string("Test connection\n"));
    while(stream_running.load()){
        for(int i=0; i<5; ++i){
            data[i] = distr(gen);
            std::cout << data[i] << " ";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        if(!(stream_paused.load())){
            std::cout << ~stream_paused.load() << " sending data...\n" ;
            ret_val = TcpSocket::sendBytes(tcp_socket, data, sizeof(int64_t)*5);
            if(ret_val<0){
                //error sending data.. so we try to reconnect once
                //if we want to be more cautious we can keep trying to connect
                TcpSocket::closeSocket(tcp_socket);
                tcp_socket = TcpSocket::socket_config(tcp_socket);
                conn = TcpSocket::connectSocket(tcp_socket, tcp_dest_ip, tcp_dest_port);
                    if(conn == -1){
                        thread_alive = false;
                        TcpSocket::closeSocket(tcp_socket);
                        return;
                    }
            }
        }
    }
    std::cout << "out of the thread while1 \n";
    thread_alive = false;
    TcpSocket::closeSocket(tcp_socket);
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

int SCPI_server::addTimestampAnswer(std::string &out_msg){
    using namespace std::chrono;

    auto now = system_clock::now();

    auto ms =
        duration_cast<milliseconds>(
            now.time_since_epoch()) % 1000;

    auto tt = system_clock::to_time_t(now);

    std::tm tm = *gmtime(&tt);

    std::ostringstream oss;

    oss << std::put_time(
                &tm,
                "%Y-%m-%dT%H:%M:%S")
        << '.'
        << std::setw(3)
        << std::setfill('0')
        << ms.count()
        << "+0000";
    out_msg +=" "+oss.str()+"\n";
    return 0;
}



int SCPI_server::parse_recv_message(const char* recv_buff, int recv_bytes, std::string &out_msg){
    std::string_view input_data(buffer, static_cast<size_t>(recv_bytes));
    for(auto& pair: this->cmds){
        if(input_data.find(pair.first) != std::string_view::npos){
            std::cout << "match! "<< pair.first << "\n";
            out_msg = pair.first+" ";
            int cmd_out = (this->*pair.second)(input_data, out_msg);

            return 1;
        }
    }
    std::cout << "no match\n";
    out_msg.assign("undefined command!");
    SCPI_server::addTimestampAnswer(out_msg);
    return -1;
}

//
//  These are the command that actually does something
//

int SCPI_server::get_value(std::string_view input_data, std::string &out_msg){
    if(~stream_running.load()){
        //out_msg = "SCPI_SERVER:GET_VALUE "+std::to_string(this->internal_value);
        out_msg += std::to_string(this->internal_value);
        SCPI_server::addTimestampAnswer(out_msg);
        return 0;
    }
    else{
        out_msg = "Streaming loop active!\n";
        return 1;
    }
}

int SCPI_server::set_value(std::string_view input_data, std::string &out_msg){
    if(~stream_running.load()){
        size_t space = input_data.find(" ");
        if(space!= std::string_view::npos){
            std::string_view s_arg = input_data.substr(space+1);
            float arg;
            auto result = std::from_chars(s_arg.data(), s_arg.data()+s_arg.size(), arg);
            if(result.ec != std::errc{}){
                out_msg += "ERROR";
                SCPI_server::addTimestampAnswer(out_msg);
                return 1;
            }
            internal_value = arg;
            out_msg += " Ok";
            SCPI_server::addTimestampAnswer(out_msg);
        }
        return 0;
    }
    else{
        out_msg += "Streaming loop active!";
        SCPI_server::addTimestampAnswer(out_msg);
        return 1;
    }
}


int SCPI_server::help(std::string_view input_data, std::string &out_msg){
    out_msg += "This is a help message";
    SCPI_server::addTimestampAnswer(out_msg);
    return 0;
}



int SCPI_server::start_stream(std::string_view msg, std::string& out_msg){
    if(!stream_running.load()){
        stream_paused = false;
        stream_running = true;
        std::cout << "initializing stream" << "\n";
        worker = std::thread( &SCPI_server::workerLoop, this);
        out_msg += "OK";
        SCPI_server::addTimestampAnswer(out_msg);
        return 0;
    }
    else{
        out_msg += "STREAM ALREADY RUNNING\n";
        return -1;
    }

}

int SCPI_server::pause_stream(std::string_view msg, std::string& out_msg){
    if(stream_running.load()){
        if(stream_paused.load()){
            out_msg = "STREAM ALREADY PAUSED\n";
            return 0;
        }
        else{
            out_msg = "SCPI_SERVER::PAUSE_STREAM OK\n";
            stream_paused = true;
            return 0;
        }
    }
    else{
        out_msg = "STREAM IS NOT INITIATED\n";
        return -1;
    }
}

int SCPI_server::resume_stream(std::string_view msg, std::string& out_msg){
    if(stream_running.load()){
        std::cout << "!strem_paused: " << !stream_paused.load() << "\n";
        if(stream_paused.load()){
            out_msg = "SCPI_SERVER::RESUME_STREAM OK\n";
            stream_paused = false;
            return 0;
        }
        else{
            out_msg = "STREAM ALREADY RESUMED\n";
            return 0;
        }
    }
    else{
        out_msg = "STREAM IS NOT INITIATED\n";
        return -1;
    }


}

int SCPI_server::close_stream(std::string_view msg, std::string& out_msg){
    if(stream_running.load()){
        out_msg = "SCPI_SERVER::CLOSE_STREAM OK\n";
        stream_running = false;
        if(worker.joinable())
            worker.join();
        return 0;
    }
    else{
        out_msg = "STREAM IS NOT INITIATED\n";
        return -1;
    }
    

}


int SCPI_server::setDestStreamIP(std::string_view input_data, std::string& out_msg){
    size_t space = input_data.find(" ");
    if(space!= std::string_view::npos){
        std::string_view s_arg = input_data.substr(space+1);
        //now I should review that this follows the standard ip format
        size_t end_str = s_arg.find("\n");
        if(end_str!= std::string::npos){
            s_arg = s_arg.substr(0, end_str);
        }
        //std::cout << "test: " << std::to_string(end_str) <<"\n";
        //std::cout << "test2: " << s_arg.substr(0, end_str) << "\n";
        tcp_dest_ip = s_arg;    //I need to check if it has \n at the end!
        std::cout << "new ip " << tcp_dest_ip <<"asdads\n";
        out_msg += "OK";
        SCPI_server::addTimestampAnswer(out_msg);
        return 0;
    }
    else{
        out_msg += "ERROR";
        SCPI_server::addTimestampAnswer(out_msg);
        return -1;
    }
}

int SCPI_server::getDestStreamIP(std::string_view msg, std::string& out_msg){
    out_msg += tcp_dest_ip;
    SCPI_server::addTimestampAnswer(out_msg);
    return 0;
}





int SCPI_server::setDestStreamPort(std::string_view input_data, std::string& out_msg){
    size_t space = input_data.find(" ");
    if(space!= std::string_view::npos){
        std::string_view s_arg = input_data.substr(space+1);
        float arg;
        auto result = std::from_chars(s_arg.data(), s_arg.data()+s_arg.size(), arg);
        if(result.ec != std::errc{}){
            out_msg += "ERROR";
            SCPI_server::addTimestampAnswer(out_msg);
            return 1;
        }
        tcp_dest_port= arg;
        out_msg += " Ok";
        SCPI_server::addTimestampAnswer(out_msg);
    }
    return 0;
}

int SCPI_server::getDestStreamPort(std::string_view msg, std::string& out_msg){
    out_msg += std::to_string(tcp_dest_port);
    SCPI_server::addTimestampAnswer(out_msg);
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
    double phase=  8.1234213;
    SCPI_server::fitsWritterFormatter(out_msg, amp, phase, now);
    std::cout << out_msg << "\n";
    return 0;
}






