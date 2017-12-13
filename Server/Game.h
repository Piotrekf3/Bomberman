#ifndef GAME_H
#define GAME_H

class Game
{
private:
	static int mapWidth;
	static int mapHeight;
	static char* serverIp;
	static int maxPlayersNumber;
	
	int gameMap[mapWidth][mapHeight];
	int players[mapWidth][mapHeight];
	int playerDescriptors[maxPlayersNumber]
}

#endif