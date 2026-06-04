#include <string>
#include <arpa/inet.h>
#include <vector>
#include <array>
#include <iostream>

namespace UdpSocket{

int socket_config(int sock=-1, int sendsize=128, int recvsize=128,
        int timeout=10, int reuse_addr=1);

int socket_creation(int timeout, int reuse_addr);
int setBuffer(int sock_fd, int sendsize, int recvsize);
sockaddr_in makeAddress(std::string_view ip, int port);
int bindSocket(int sock_fd, std::string_view ip, int port);
int closeSocket(int sock_fd);

//this is a raw implementation of the sendto (ie wo containers)
int sendTo(int sock_fd,
        const void *data,
        size_t bytes,
        const sockaddr_in &addr);

int recvFrom(int sock_fd,
        void *data,
        size_t max_bytes,
        sockaddr_in &src_addr);


template <typename T>
int sendVectorDataTo(int sock_fd, const std::vector<T> &data, const sockaddr_in& addr){
    return sendTo(sock_fd, data.data(), data.size()*sizeof(T), addr);
}

template <typename T, std::size_t N>
int sendArrayDataTo(int sock_fd, const std::array<T,N> &data, const sockaddr_in& addr){
    return sendTo(sock_fd, data.data(), N*sizeof(T), addr);
}

template <typename T, std::size_t N>
int sendCArrayDataTo(int sock_fd,  const T (&arr)[N], const sockaddr_in& addr){
    return sendTo(sock_fd, arr, N*sizeof(T), addr);
}

template <std::size_t N>
int sendStringDataTo(int sock_fd, const char (&str)[N], const sockaddr_in& addr){
    return sendTo(sock_fd, str, N-1, addr);
}

}
