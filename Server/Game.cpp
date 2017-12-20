#include "Game.h"
char* Game::serverIp = "127.0.0.1"; 
int Game::mapWidth=10;
int Game::mapHeight=10;
int Game::maxPlayersNumber=2;

ssize_t Game::readData(int fd, char * buffer, ssize_t buffsize) {
    auto ret = read(fd, buffer, buffsize);
    if(ret==-1) error(1,errno, "read failed on descriptor %d", fd);
    return ret;
}

void Game::writeData(int fd,const char * buffer, ssize_t count) {
    auto ret = write(fd, buffer, count);
    if(ret==-1) error(1, errno, "write failed on descriptor %d", fd);
    if(ret!=count) error(0, errno, "wrote less than requested to descriptor %d (%ld/%ld)", fd, count, ret);
}

void Game::sendMoveToAll(int player,Pair from, Pair to)
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

void Game::sendMapChange(int sd, Pair where, int value)
{
		string buffer;
		buffer = to_string(where.x) + ";" + to_string(where.y) + ";" + to_string(value);
		writeData(sd ,buffer.c_str(), 255);
}

void Game::sendGameMap(int sd)
{
	for(int i=0;i<mapWidth;i++)
		for(int j=0;j<mapHeight;j++)
		{
			sendMapChange(sd,Pair(i,j),gameMap[i][j]);
		}
}


Pair Game::getPlayerPosition(int player)
{
    for(int i=0; i<mapWidth; i++)
        for(int j=0; j<mapHeight; j++)
        {
            if(players[i][j]==player)
                return Pair(i,j);
        }
    return Pair(0,0);
}

bool Game::validateMove(int player, char * direction)
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

void Game::makeMove(int player, char * direction)
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

void Game::initGameMap()
{
    for(int i=0; i<mapHeight; i++)
        for(int j=0; j<mapWidth; j++)
            gameMap[i][j]=0;
	gameMap[1][2]=1;
}

void Game::initPlayers()
{
    for(int i=0; i<mapHeight; i++)
        for(int j=0; j<mapWidth; j++)
            players[i][j]=0;
}

void Game::clientThread(int playerNumber)
{
	cout<<playerNumber<<endl;
    ssize_t bufsize = 255;
    char buffer[bufsize];
    //start game
	strcpy(buffer,"start");
    writeData(playerDescriptors[playerNumber], buffer, bufsize);
	sleep(1);
	sendGameMap(playerDescriptors[playerNumber]);
    //send players positions
    Pair playerPosition = getPlayerPosition(playerDescriptors[playerNumber]);
    sendMoveToAll(playerDescriptors[playerNumber],playerPosition,playerPosition);
	sleep(1);

    while(!threadStop[playerNumber])
    {
		cout<<"reading"<<endl;
		cout<<"threadStop"<<threadStop[playerNumber]<<endl;
        readData(playerDescriptors[playerNumber], buffer, bufsize);
        cout<<buffer<<endl;
        if(strcmp(buffer,"left")==0 || strcmp(buffer,"right")==0
				|| strcmp(buffer,"up") || strcmp(buffer,"down"))
        {
            makeMove(playerDescriptors[playerNumber],buffer);
            strcpy(buffer,"null");
        }
    }
}

Game::Game(int descriptors[]) : gameMap(mapWidth, vector<int>(mapHeight)),
	players(mapWidth, vector<int>(mapHeight)),
	playerDescriptors(maxPlayersNumber),
	threadStop(maxPlayersNumber),
	t(maxPlayersNumber),
	descriptorsMutex(maxPlayersNumber)	
{
	cout<<"thread="<<threadStop[0]<<endl;
	cout<<"konstruktor"<<endl;
	initGameMap();
	initPlayers();
	for(int i=0;i<maxPlayersNumber;i++)
		this->playerDescriptors[i] = descriptors[i];
	for(int i=0; i<maxPlayersNumber; i++)
	{
		threadStop[i]=false;
		if(i==0)
			players[0][0]=playerDescriptors[i];
		else if(i==1)
			players[mapWidth-1][mapHeight-1]=playerDescriptors[i];
		else if(i==2)
			players[0][mapHeight-1]=playerDescriptors[i];
		else if(i==3)
			players[mapWidth-1][0]=playerDescriptors[i];
	}
	for(int i=0; i<maxPlayersNumber;i++)
	{
		t[i] = thread(&Game::clientThread,this,i);
	}
}

Game::~Game()
{
	cout<<"end of game"<<endl;
	for(int i=0;i<1;i++)
	{
		threadStop[i]=true;
		if(t[i].joinable())
			t[i].join();
		close(playerDescriptors[i]);
	}

}

void Game::loadConfig()
{
	mapWidth=10;
	mapHeight=10;
	strcpy(serverIp,"127.0.0.1");
	maxPlayersNumber=2;
}
