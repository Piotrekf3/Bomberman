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
#include "Game.h"
using namespace std;

const int mapWidth = 10;
const int mapHeight = 10;
int gameMap[mapWidth][mapHeight];
int players[mapWidth][mapHeight];

const char * serverIp="192.168.0.19";
const int maxPlayersNumber=2;
int playerDescriptors[maxPlayersNumber];

mutex gameStart;
mutex readStart;
mutex descriptorsMutex[maxPlayersNumber];

class Pair
{
public:
    int x;
    int y;
    Pair(int x, int y)
    {
        this->x = x;
        this->y = y;
    }
};


ssize_t readData(int fd, char * buffer, ssize_t buffsize) {
    auto ret = read(fd, buffer, buffsize);
    if(ret==-1) error(1,errno, "read failed on descriptor %d", fd);
    return ret;
}

void writeData(int fd,const char * buffer, ssize_t count) {
    auto ret = write(fd, buffer, count);
    if(ret==-1) error(1, errno, "write failed on descriptor %d", fd);
    if(ret!=count) error(0, errno, "wrote less than requested to descriptor %d (%ld/%ld)", fd, count, ret);
}

//wysyla ruch do wszystkich graczy
//mozna zmienic na string
void sendMoveToAll(int player,Pair from, Pair to)
{
    char result[40];
    string splayer = to_string(player);
    string fx = to_string(from.x);
    string fy = to_string(from.y);
    string tx = to_string(to.x);
    string ty = to_string(to.y);
    snprintf(result,sizeof(result), "%s;%s;%s;%s;%s",splayer.c_str(),
             fx.c_str(),fy.c_str(),tx.c_str(),ty.c_str());
	for(int i=0;i<maxPlayersNumber;i++)
	{
		descriptorsMutex[i].lock();
		writeData(playerDescriptors[i],result,255);
		descriptorsMutex[i].unlock();
	}
}

//przesyla zmianę na mapie do graczy
void sendMapChange(int sd, Pair where, int value)
{
		string buffer;
		buffer = to_string(where.x) + ";" + to_string(where.y) + ";" + to_string(value);
		cout<<buffer<<endl;
		writeData(sd ,buffer.c_str(), 255);
}

void sendGameMap(int sd)
{
	for(int i=0;i<mapWidth;i++)
		for(int j=0;j<mapHeight;j++)
		{
			sendMapChange(sd,Pair(i,j),gameMap[i][j]);
		}
}

//zwraca pozycje gracza na planszy
Pair getPlayerPosition(int player)
{
    for(int i=0; i<mapWidth; i++)
        for(int j=0; j<mapHeight; j++)
        {
            if(players[i][j]==player)
                return Pair(i,j);
        }
    return Pair(0,0);
}

bool validateMove(int player, char * direction)
{
	Pair position = getPlayerPosition(player);
	string dir = direction;
	if(dir=="left" && (position.y-1) >= 0 && players[position.x][position.y-1]==0
			&& gameMap[position.x][position.y-1]==0)
	{
		return true;
	}
	else if(dir=="right" && (position.y+1)<10 && players[position.x][position.y+1]==0
			&& gameMap[position.x][position.y+1]==0)
	{
		return true;
	}
	else if(dir=="up" && (position.x-1)>=0 && players[position.x-1][position.y]==0
			&& gameMap[position.x-1][position.y]==0)
	{
		return true;
	}
	else if(dir=="down" && (position.x+1)<10 && players[position.x+1][position.y]==0
			&& gameMap[position.x+1][position.y]==0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void makeMove(int player, char * direction)
{
    Pair position = getPlayerPosition(player);
    int i = position.x;
    int j = position.y;
	if(!validateMove(player,direction))
		return;
    players[i][j]=0;
    if(strcmp(direction,"left")==0)
    {
        players[i][j-1]=player;
		sendMoveToAll(player,Pair(i,j),Pair(i,j-1));
    }
    else if(strcmp(direction,"right")==0)
    {
        players[i][j+1]=player;
		sendMoveToAll(player,Pair(i,j),Pair(i,j+1));
    }
    else if(strcmp(direction,"up")==0)
    {
        players[i-1][j]=player;
		sendMoveToAll(player,Pair(i,j),Pair(i-1,j));
    }
    else if(strcmp(direction,"down")==0)
    {
        players[i+1][j]=player;
		sendMoveToAll(player,Pair(i,j),Pair(i+1,j));
    }
}

void readThread(int sd)
{
	readStart.lock();
	readStart.unlock();
    while(1)
    {
        ssize_t bufsize = 255;
        char buffer[bufsize];
        readData(sd, buffer, bufsize);
        cout<<buffer<<endl;
        if(strcmp(buffer,"left")==0 || strcmp(buffer,"right")==0
				|| strcmp(buffer,"up") || strcmp(buffer,"down"))
        {
            makeMove(sd,buffer);
 /*           for(int i=0; i<mapWidth; i++)
            {
                for(int j=0; j<mapHeight; j++)
                    cout<<players[i][j];
                cout<<endl;
            }*/
            strcpy(buffer,"null");
        }
    }
}

void writeThread(int sd)
{
    ssize_t bufsize = 255;
    char buffer[bufsize];
    //start game
	strcpy(buffer,"start");
    writeData(sd, buffer, bufsize);
	sleep(1);
	sendGameMap(sd);
    //send players positions
    Pair playerPosition = getPlayerPosition(sd);
    sendMoveToAll(sd,playerPosition,playerPosition);
	sleep(1);
	readStart.unlock();
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
	gameMap[1][2]=1;
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
	readStart.lock();
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
            playerDescriptors[i]=cd;
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
