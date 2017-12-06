#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
using namespace std;

const char * serverIp="192.168.0.19";

int main(int argc, char **argv) {
    int sd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);

    sockaddr_in saddr;
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(2500);
    saddr.sin_addr.s_addr=inet_addr(serverIp);

    bind(sd,(sockaddr*) &saddr,sizeof(saddr));
    listen(sd,1);

    while(1)
        if(accept(sd, nullptr, nullptr)>=0)
            cout<<"poloczenie\n";

    close(sd);
    return 0;
}
