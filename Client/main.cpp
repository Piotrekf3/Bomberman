#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

const char * ip = "127.0.0.1";

void clientRead(int sd)
{
    cout<<"clientRead\n";
}

void clientWrite(int sd)
{
    cout<<"clientWrite\n";
}

int main() {
    int sd = socket(AF_INET,SOCK_STREAM,0);

    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(2500);
    saddr.sin_addr.s_addr = inet_addr(ip);

    thread read;
    thread write;
    int cd = connect(sd,(sockaddr*) &saddr,sizeof(saddr));
    if(cd==0)
    {
        read = thread(clientRead,sd);
        write = thread(clientWrite,sd);
    	read.join();
    	write.join();
    }
    return 0;
}
