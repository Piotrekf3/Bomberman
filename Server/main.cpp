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
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Game.h"
#include "Pair.h"
using namespace std;

//tablica gier
vector <unique_ptr<Game>> games;
mutex gamesMutex;

void gameEnd()
{
	while(1)
	{
		sleep(1);
		lock_guard<mutex> lock(gamesMutex);
		for(auto it=games.begin();it!=games.end();)
		{
			if((*it)->getPlayersCount()<=1)
				it = games.erase(it);
			else
				++it;
		}
	}
}

void checkConnected(int& connectedNumber, int playerDescriptors[])
{
	int n=connectedNumber;
	for(int i=0;i<n;i++)
	{
		string buffer;
		Game::writeData(playerDescriptors[i],"check");
		if(Game::readData(playerDescriptors[i],buffer)==0)
		{
			cout<<" disconnected\n";
			playerDescriptors[i]=0;
			connectedNumber--;
		}
	}
}

int main(int argc, char **argv) {

	//wczytanie konfiguracji
	Game::loadConfig();
    int sd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    sockaddr_in saddr;
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(2500);
    saddr.sin_addr.s_addr=inet_addr(Game::getServerIp().c_str());

    const int one = 1;
    setsockopt(sd,SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if(bind(sd,(sockaddr*) &saddr,sizeof(saddr))<0)
	{
		cout<<"bind failed"<<endl;
		return 1;
	}
    listen(sd,Game::getMaxPlayersNumber());


	int playerDescriptors[Game::getMaxPlayersNumber()];
	thread t(gameEnd);

    int i=0;
    while(1)
    {
    	int cd = accept(sd, nullptr, nullptr);
		checkConnected(i,playerDescriptors);
        if(cd>=0 && i<Game::getMaxPlayersNumber())
        {
            playerDescriptors[i]=cd;
			i++;
        }
        if(i==Game::getMaxPlayersNumber())
        {
            //start gry
			cout<<"start gry\n";
			lock_guard<mutex> lock(gamesMutex);
			games.push_back(unique_ptr<Game>(new Game(playerDescriptors)));
			i=0;
        }
    }
    close(sd);
    return 0;
}
