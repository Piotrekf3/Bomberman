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
#include <vector>
#include <atomic>

#include "Pair.h"

using namespace std;

class Game
{
private:
	//static
//	const static char * configFile="config";
	static int mapWidth;
	static int mapHeight;
	static char* serverIp;
	static int maxPlayersNumber;
	
	//variables
	vector<vector<int>> gameMap;
	vector<vector<int>> players;
	vector<int> playerDescriptors;

	vector<atomic<bool>> threadStop;
    vector<thread> t;

	vector<mutex> descriptorsMutex;

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
	void clientThread(int sd);
	void initGameMap();
	void initPlayers();

public:
	Game(int descriptors[]);
	~Game();
	static void loadConfig();
	static const char * getServerIp(){return serverIp;};
	static int getMaxPlayersNumber() {return maxPlayersNumber;};
};

#endif
