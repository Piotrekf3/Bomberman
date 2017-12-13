#ifndef GAME_H
#define GAME_H

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

#include "Pair.h"

using namespace std;

class Game
{
private:
	//static
	const static int mapWidth = 10;
	const static int mapHeight = 10;
	const static char* serverIp;
	const static int maxPlayersNumber = 2;
	
	//variables
	int gameMap[mapWidth][mapHeight];
	int players[mapWidth][mapHeight];
	int playerDescriptors[maxPlayersNumber];

    thread t[maxPlayersNumber*2];

	mutex readStart;
	mutex descriptorsMutex[maxPlayersNumber];

	//methods
	ssize_t readData(int fd, char * buffer, ssize_t buffsize);
	void writeData(int fd,const char * buffer, ssize_t count);
	void sendMoveToAll(int player,Pair from, Pair to);
	//przesyla zmianę na mapie do graczy
	void sendMapChange(int sd, Pair where, int value);
	void sendGameMap(int sd);
	Pair getPlayerPosition(int player);
	bool validateMove(int player, char * direction);
	void makeMove(int player, char * direction);
	void readThread(int sd);
	void writeThread(int sd);
	void initGameMap();
	void initPlayers();

public:
	Game(int descriptors[]);
};

#endif
