#include <iostream>
#include "../includes/TcpSocket.h"
#include <sys/socket.h>

std::string_view host {"10.0.33.133"};
int port {12234};

int test_sending(){
    int sock_fd = TcpSocket::socket_config();
    std::cout << "sock_fd " << sock_fd <<"\n";
    std::cout << "trying to connect ("<< host << "," << port << ")\n";
    if(TcpSocket::connectSocket(sock_fd, host, port)){
        std::cout << "error connecting\n";
        return -1;
    }
    std::cout << "trying to send data\n";
    std::string msg{"asdadsq\n"};
    
    int sent_bytes = TcpSocket::sendStringData(sock_fd, msg);
    std::cout << "sent "<< sent_bytes << "\n";

    std::vector<char> test_vector {'H','e','l','l','o','\n'};
    sent_bytes = TcpSocket::sendVectorData(sock_fd, test_vector);
    

    std::cout << "closing socket...";
    TcpSocket::closeSocket(sock_fd);
    std::cout << "done\n";
    return 0;
}


int main(){
    test_sending();
    return 0;
}
