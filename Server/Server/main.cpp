#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
using namespace std;

const int mapWidth = 10;
const int mapHeight = 10;
int gameMap[mapWidth][mapHeight];

const char * serverIp="192.168.0.19";
const int maxPlayersNumber=2;

mutex gameStart;

void clientThread(int sd)
{
    gameStart.lock();
    cout<<"client "<<sd<<" thread\n";
    gameStart.unlock();
}

void initGameMap()
{
    for(int i=0;i<mapHeight;i++)
        for(int j=0;j<mapWidth;j++)
            gameMap[i][j]=0;
}

int main(int argc, char **argv) {
    thread t[maxPlayersNumber];
    int sd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);

    sockaddr_in saddr;
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(2500);
    saddr.sin_addr.s_addr=inet_addr(serverIp);

    bind(sd,(sockaddr*) &saddr,sizeof(saddr));
    listen(sd,maxPlayersNumber);

    initGameMap();
    /*for(int i=0;i<10;i++)
    {
        for(int j=0;j<10;j++)
            cout<<gameMap[i][j];
        cout<<endl;
    }*/

    int cd;
    int i=0;
    gameStart.lock();
    while(1)
    {
        cd = accept(sd, nullptr, nullptr);
        if(cd>=0 && i<maxPlayersNumber)
        {
                t[i] = thread(clientThread,cd);
                i++;
        }
        if(i==maxPlayersNumber)
        {
            //start gry
            gameStart.unlock();
        }
    }

    close(sd);
    return 0;
}
