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
#include <fstream>
#include <sys/socket.h>
#include <sys/types.h>
#include "Pair.h"

using namespace std;

class Game
{
private:
	//static
	const static string configFile;
	const static string mapFile;
	static int mapWidth;
	static int mapHeight;
	static string serverIp;
	static int maxPlayersNumber;
	
	//variables
	vector<vector<int>> gameMap;
	mutex mapMutex;
	vector<vector<int>> players;
	mutex playersMutex;
	vector<int> playerDescriptors;

	vector<atomic<bool>> threadStop;
    vector<thread> t;

	vector<mutex> descriptorsMutex;
	vector<atomic<int>> bombs;
	vector<thread> bombThreads;

	atomic<int> playersCount;

	//methods
	void sendMoveToAll(int player,Pair from, Pair to);
	void sendMapChange(int sd, Pair where, int value);
	void sendGameMap(int sd);
	Pair getPlayerPosition(int player);
	bool validateMove(int player, const string& direction);
	void makeMove(int player, const string& direction);
	void clientThread(int sd);
	void initPlayers();
	void bombThread(Pair position, int playerNumber);
	void placeBomb(int playerNumber);

public:
	explicit Game(int descriptors[]);
	~Game();
	int getPlayersCount(){return playersCount;};
	void endSignal(){playersCount=0;};
	static void loadConfig();
	void loadMap();
	static int getMaxPlayersNumber() {return maxPlayersNumber;};
	static const string getServerIp() {return serverIp;};
	static int getMapWidth() {return mapWidth;};
	static int getMapHeight() {return mapHeight;};
	static string getExecutablePath();
	static ssize_t readData(int fd, string& buffer);
	static ssize_t writeData(int fd,const string& buffer);
};

#endif
