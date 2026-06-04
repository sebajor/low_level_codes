#include <string>
#include <arpa/inet.h>
#include <vector>
#include <iostream>

namespace TcpSocket{
int socket_config(int sock=-1, int sndsize=128, int recvsize=128,
        int timeout=10, int reuse_addr=1);

int socket_creation(int timeout, int reuse_addr);

int setBuffer(int sock_fd, int sendsize, int recvsize);

int connectSocket(int sock_fd, std::string_view host, int port);

int bindListenSocket(int sock_fd, std::string_view ip, int port, int backlog=32);

int acceptConnection(int sock_fd, sockaddr &client_addr);

int closeSocket(int sock_fd);


int sendBytes(int sock_fd, const void* data, size_t nbytes);

int recvBytes(int sock_fd, void* data, size_t nbytes, bool keep=true);

template<typename T>
int sendVectorData(int sock_fd, const std::vector<T> &data){
    return sendBytes(sock_fd, data.data(), data.size()*sizeof(T));

}

template<typename T, std::size_t N>
int sendArrayData(int sock_fd, const std::array<T,N> &arr){
    return sendBytes(sock_fd, arr.data(), sizeof(T)*N);
}

int sendStringData(int sock_fd, const std::string &str);

template<typename T>
int recvVectorData(int sock_fd, std::vector<T> &recv_vec, int recv_len){
    recv_vec.resize(recv_len);
    return recvBytes(sock_fd, recv_vec.data(), recv_len*sizeof(T));
}

template<typename T, std::size_t N>
int recvArrayData(int sock_fd, const std::array<T,N> &arr){
    return recvBytes(sock_fd, arr.data(), sizeof(T)*N);
}






}
