#include <iostream>
#include <error.h>
#include <errno.h>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

const int mapWidth = 10;
const int mapHeight = 10;
int gameMap[mapWidth][mapHeight];
int players[mapWidth][mapHeight];

const char * serverIp="127.0.0.1";
const int maxPlayersNumber=2;

mutex gameStart;

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

void makeMove(int player, char * direction)
{
	cout<<"player="<<player<<endl;
	for(int i=0;i<mapWidth;i++)
		for(int j=0;j<mapHeight;j++)
			if(players[i][j]==player)
			{
				cout<<"i="<<i<<" j="<<j<<endl;
				players[i][j]=0;
				if(strcmp(direction,"left")==0)
				{
					players[i][j-1]=player;
				}
				else if(strcmp(direction,"right")==0)
				{
					players[i][j+1]=player;
				}
				break;
			}
}

void readThread(int sd)
{
    while(1)
    {
        ssize_t bufsize = 255, received;
        char buffer[bufsize];
        received = readData(sd, buffer, bufsize);
        //writeData(1, buffer, received);
		cout<<buffer<<endl;
		if(strcmp(buffer,"left")==0 || strcmp(buffer,"right")==0)
		{
			makeMove(sd,buffer);
			for(int i=0;i<mapWidth;i++)
			{
				for(int j=0;j<mapHeight;j++)
					cout<<players[i][j];
				cout<<endl;
			}
			strcpy(buffer,"null");
		}
    }
}

void writeThread(int sd)
{

    //ssize_t bufsize2 = 255, received2;
    //char buffer2[bufsize2];
	writeData(sd, "start", 6);
	
}

void clientThread(int sd)
{
    gameStart.lock();
    cout<<"client "<<sd<<" thread\n";
    gameStart.unlock();
    //start gry
    thread read = thread(readThread,sd);
    thread write = thread(writeThread,sd);

    read.join();
    write.join();
}

void initGameMap()
{
    for(int i=0; i<mapHeight; i++)
        for(int j=0; j<mapWidth; j++)
            gameMap[i][j]=0;
}

void initPlayers()
{
    for(int i=0; i<mapHeight; i++)
        for(int j=0; j<mapWidth; j++)
            players[i][j]=0;
}

int main(int argc, char **argv) {
    thread t[maxPlayersNumber];
    int sd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);

    sockaddr_in saddr;
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(2500);
    saddr.sin_addr.s_addr=inet_addr(serverIp);

	const int one = 1;
	setsockopt(sd,SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    bind(sd,(sockaddr*) &saddr,sizeof(saddr));
    listen(sd,maxPlayersNumber);

    initGameMap();
	initPlayers();
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
			if(i==0)
				players[0][0]=cd;
			else if(i==1)
				players[9][9]=cd;
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
