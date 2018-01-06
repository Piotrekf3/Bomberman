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
    bind(sd,(sockaddr*) &saddr,sizeof(saddr));
    listen(sd,Game::getMaxPlayersNumber());

	//tablica gier
	vector <unique_ptr<Game>> games;
	int playerDescriptors[Game::getMaxPlayersNumber()];

    int cd;
    int i=0;
    while(1)
    {
        cd = accept(sd, nullptr, nullptr);
        if(cd>=0 && i<Game::getMaxPlayersNumber())
        {
			cout<<"cd="<<cd<<endl;
            playerDescriptors[i]=cd;
			i++;
        }
        if(i==Game::getMaxPlayersNumber())
        {
            //start gry
			cout<<"start gry\n";
			games.push_back(unique_ptr<Game>(new Game(playerDescriptors)));
			i=0;
        }
    }
	cout<<"main koniec"<<endl;
    close(sd);
    return 0;
}
