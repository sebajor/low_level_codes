#include "../includes/UdpSocket.h"
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <vector>
#include <netdb.h>
#include <unistd.h>



namespace UdpSocket {

int socket_config(int sock, int sendsize, int recvsize, int timeout,
        int reuse_addr){
    int sock_fd = -1;
    if(sock==-1)
        sock_fd = socket_creation(timeout, reuse_addr);
    else
        sock_fd = sock;
    setBuffer(sock_fd, sendsize, recvsize);
    return sock_fd;
}

int socket_creation(int timeout, int reuse_addr){
    struct timeval sock_timeout {};
    const int enable = 1;

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_fd<0)
        return -1;
    if(timeout<0)
        timeout=0;
    sock_timeout.tv_sec = timeout;

    setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, &sock_timeout, sizeof(sock_timeout));
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &sock_timeout, sizeof(sock_timeout));
    if(reuse_addr){
        setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
        setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
    }
    return sock_fd;
}

int setBuffer(int sock_fd, int sendsize, int recvsize){
    int send = setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &sendsize, sizeof(int));
    int recv = setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &sendsize, sizeof(int));
    return (send|recv);
}

sockaddr_in makeAddress(std::string_view ip, int port){
    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if(inet_pton(AF_INET, ip.data(), &addr.sin_addr) <= 0){
        std::cout << "Error parsing IP address\n";
    }
    return addr;
}


int bindSocket(int sock_fd, std::string_view ip, int port){
    sockaddr_in addr = makeAddress(ip, port);
    if(bind(sock_fd, (sockaddr*)&addr, sizeof(addr))){
        std::cout << "UDP bind error\n";
        return -1;
    }
    return 0;
}

int closeSocket(int sock_fd){
    if (sock_fd >= 0)
        close(sock_fd);
    return 0;

}


int sendTo(int sock_fd,
           const void *data,
           size_t bytes,
           const sockaddr_in &addr)
{
    return sendto(sock_fd,
                  data,
                  bytes,
                  0,
                  (const sockaddr *)&addr,
                  sizeof(addr));
}

int recvFrom(int sock_fd,
             void *buffer,
             size_t max_bytes,
             sockaddr_in &src_addr)
{
    socklen_t len = sizeof(src_addr);
    return recvfrom(sock_fd,
                    buffer,
                    max_bytes,
                    0,
                    (sockaddr *)&src_addr,
                    &len);
}



}


