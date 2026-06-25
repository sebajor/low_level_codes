#include <iostream>
#include "../includes/SCPI_server.h"
#include <string>

int main(){
    int port {12234};
    //std::string_view host {"10.0.33.133"};
    std::string_view host {"0.0.0.0"};
    int recv_bytes = 0;
    std::string ans;


    std::cout << "creating server..";
    SCPI_server serv(host, port);
    std::cout << "done\n";
    
    while(1){
        recv_bytes = serv.check_message();
        if(recv_bytes!=0){
            std::cout << "got some message\n";
            for(int i=0; i<recv_bytes; ++i){
                std::cout << serv.buffer[i];
            }
            serv.parse_recv_message(serv.buffer, recv_bytes, ans);
            std::cout << "answering:\n";
            std::cout << ans << "\n";
            serv.answer_request(ans);
        }
    }
    return 0;
}
