#include <iostream>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <cstdlib>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

const char * ip = "127.0.0.1";

ssize_t readData(int fd, char * buffer, ssize_t buffsize) {
    auto ret = read(fd, buffer, buffsize);
    if(ret==-1) error(1,errno, "read failed on descriptor %d", fd);
    return ret;
}

void writeData(int fd, char * buffer, ssize_t count) {
    auto ret = write(fd, buffer, count);
    if(ret==-1) error(1, errno, "write failed on descriptor %d", fd);
    if(ret!=count) error(0, errno, "wrote less than requested to descriptor %d (%ld/%ld)", fd, count, ret);
}

void clientRead(int sd)
{
    ssize_t bufsize = 255, received;
    char buffer[bufsize];
    while(strcmp(buffer,"start")!=0)
	    readData(sd, buffer, bufsize);
    cout<<"start gry\n";
    while(1)
    {
        received = readData(sd, buffer, bufsize);
        writeData(1, buffer, received);
    }

}

void clientWrite(int sd)
{
    while(1)
    {
        ssize_t bufsize = 255, received;
        char buffer[bufsize];
        received = readData(0, buffer, bufsize);
        writeData(sd, buffer, received);
    }
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
