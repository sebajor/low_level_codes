#include "../includes/UdpSocket.h"
#include <iostream>
#include <sys/socket.h>


std::string_view host {"10.0.33.133"};
int port {12234};

int main(){
    int sock = UdpSocket::socket_config();
    if(UdpSocket::bindSocket(sock, host, port)){
        std::cout << "Error binding socket\n";
        return -1;
    }
    sockaddr_in client_addr;
    char buffer[2048] {0};
    int recv_bytes = 0;
    while(1){
        std::cout << "waiting for some connection...";
        recv_bytes = UdpSocket::recvFrom(sock, 
                buffer, 
                sizeof(buffer),
                client_addr);
        if(recv_bytes>0){
            for(int i=0; i<recv_bytes; ++i){
                std::cout << buffer[i];
            }
            UdpSocket::sendTo(sock, buffer, recv_bytes, client_addr);
            if(buffer[0] == 'q'){
                std::cout << "getting out of the while loop\n";
                break;
            }
        }
    }
    std::cout << "closing socket....\n";
    UdpSocket::closeSocket(sock);
    return 0;
}
