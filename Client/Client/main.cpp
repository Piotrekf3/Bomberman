#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

const char * ip = "192.168.0.19";

int main() {
    int sd = socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(2500);
    saddr.sin_addr.s_addr = inet_addr(ip);
    int cd = connect(sd,(sockaddr*) &saddr,sizeof(saddr));
    cout<<cd<<endl;
    return 0;
}
