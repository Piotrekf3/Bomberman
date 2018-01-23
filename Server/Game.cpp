#include "Game.h"

//config file
const string Game::configFile = Game::getExecutablePath() + "config";
const string Game::mapFile = Game::getExecutablePath() + "map";

//default config
string Game::serverIp = "127.0.0.1";
int Game::mapWidth=10;
int Game::mapHeight=10;
int Game::maxPlayersNumber=2;

ssize_t Game::readData(int fd, string& buffer) {
    char cbuffer;
    buffer="";
    ssize_t ret=1;
    while(ret)
    {
        ret=read(fd,&cbuffer,1);
        if(ret==0)
            return 0;
        else if(cbuffer=='!')
            break;
        else
            buffer+=string(1,cbuffer);
    }
    return buffer.length();
}

ssize_t Game::writeData(int fd,const string& buffer) {
    ssize_t ret = send(fd, (buffer+"!").c_str(),buffer.length()+1,MSG_NOSIGNAL);
    if(ret==-1) error(1, errno, "write failed on descriptor %d", fd);
    return ret;
}

//if to = (-1,-1) player is killed
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
    for(int i=0; i<maxPlayersNumber; i++)
    {
        descriptorsMutex[i].lock();
        writeData(playerDescriptors[i],result);
        descriptorsMutex[i].unlock();
    }
}

void Game::sendMapChange(int sd, Pair where, int value)
{
    string buffer;
    buffer = to_string(where.x) + ";" + to_string(where.y) + ";" + to_string(value);
    writeData(sd ,buffer);
}

void Game::sendGameMap(int sd)
{
    for(int i=0; i<mapWidth; i++)
        for(int j=0; j<mapHeight; j++)
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
    return Pair(-1,-1);
}

bool Game::validateMove(int player, const string& direction)
{
    Pair position = getPlayerPosition(player);
    if(position.x==-1 && position.y==-1)
        return false;
    else if(direction=="left" && (position.y-1) >= 0 && players[position.x][position.y-1]==0
            && gameMap[position.x][position.y-1]%3==0)
    {
        return true;
    }
    else if(direction=="right" && (position.y+1)<mapWidth && players[position.x][position.y+1]==0
            && gameMap[position.x][position.y+1]%3==0)
    {
        return true;
    }
    else if(direction=="up" && (position.x-1)>=0 && players[position.x-1][position.y]==0
            && gameMap[position.x-1][position.y]%3==0)
    {
        return true;
    }
    else if(direction=="down" && (position.x+1)<mapHeight && players[position.x+1][position.y]==0
            && gameMap[position.x+1][position.y]%3==0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void Game::makeMove(int player, const string& direction)
{
    unique_lock<mutex> lock1(mapMutex,defer_lock);
    unique_lock<mutex> lock2(playersMutex,defer_lock);
    lock(lock1,lock2);
    Pair position = getPlayerPosition(player);
    int i = position.x;
    int j = position.y;
    if(!validateMove(player,direction))
        return;
    players[i][j]=0;
    if(direction=="left")
    {
        players[i][j-1]=player;
        sendMoveToAll(player,Pair(i,j),Pair(i,j-1));
    }
    else if(direction=="right")
    {
        players[i][j+1]=player;
        sendMoveToAll(player,Pair(i,j),Pair(i,j+1));
    }
    else if(direction=="up")
    {
        players[i-1][j]=player;
        sendMoveToAll(player,Pair(i,j),Pair(i-1,j));
    }
    else if(direction=="down")
    {
        players[i+1][j]=player;
        sendMoveToAll(player,Pair(i,j),Pair(i+1,j));
    }
}

void Game::initPlayers()
{
    for(int i=0; i<mapHeight; i++)
        for(int j=0; j<mapWidth; j++)
            players[i][j]=0;
}

void Game::bombThread(Pair position, int playerNumber)
{
    sleep(3);
    gameMap[position.x][position.y]=0;
    vector<Pair> destroyedBlocks;
    for(int i=position.y; i<mapWidth; i++)
    {
        if(gameMap[position.x][i]==2)
            break;
        else if(gameMap[position.x][i]==1)
        {
            destroyedBlocks.push_back(Pair(position.x,i));
            gameMap[position.x][i]=0;
        }
        else if(players[position.x][i]!=0)
        {
            int player = players[position.x][i];
            players[position.x][i]=0;
            if(!threadStop[0])
                sendMoveToAll(player,Pair(position.x,i),Pair(-1,-1));
            playersCount--;
        }
    }
    for(int i=position.y; i>=0; i--)
    {
        if(gameMap[position.x][i]==2)
            break;
        else if(gameMap[position.x][i]==1)
        {
            destroyedBlocks.push_back(Pair(position.x,i));
            gameMap[position.x][i]=0;
        }
        else if(players[position.x][i]!=0)
        {
            int player = players[position.x][i];
            players[position.x][i]=0;
            if(!threadStop[0])
                sendMoveToAll(player,Pair(position.x,i),Pair(-1,-1));
            playersCount--;
        }
    }

    for(int i=position.x; i<mapHeight; i++)
    {
        if(gameMap[i][position.y]==2)
            break;
        else if(gameMap[i][position.y]==1)
        {
            destroyedBlocks.push_back(Pair(i,position.y));
            gameMap[i][position.y]=0;
        }
        else if(players[i][position.y]!=0)
        {
            int player = players[i][position.y];
            players[i][position.y]=0;
            if(!threadStop[0])
                sendMoveToAll(player,Pair(i,position.y),Pair(-1,-1));
            playersCount--;
        }
    }

    for(int i=position.x; i>=0; i--)
    {
        if(gameMap[i][position.y]==2)
            break;
        else if(gameMap[i][position.y]==1)
        {
            destroyedBlocks.push_back(Pair(i,position.y));
            gameMap[i][position.y]=0;
        }
        else if(players[i][position.y]!=0)
        {
            int player = players[i][position.y];
            players[i][position.y]=0;
            if(!threadStop[0])
                sendMoveToAll(player,Pair(i,position.y),Pair(-1,-1));
            playersCount--;
        }
    }
    if(!threadStop[0])
    {
        for(int i=0; i<maxPlayersNumber; i++)
        {
            sendMapChange(playerDescriptors[i],position,0);
            for(auto&& it : destroyedBlocks)
            {
                sendMapChange(playerDescriptors[i],it,0);
            }
        }
    }
    bombs[playerNumber]--;
}

void Game::placeBomb(int playerNumber)
{
    Pair position = getPlayerPosition(playerDescriptors[playerNumber]);
    if(gameMap[position.x][position.y]==0 && bombs[playerNumber]<3)
    {
        gameMap[position.x][position.y]=3;

        int index=3*playerNumber + bombs[playerNumber];
        bombThreads[index] = thread(&Game::bombThread,this,position,playerNumber);
        bombThreads[index].detach();
        bombs[playerNumber]++;

        for(int i=0; i<maxPlayersNumber; i++)
        {
            sendMapChange(playerDescriptors[i],position,3);
        }
    }
}

void Game::clientThread(int playerNumber)
{
    string buffer;
    //start game
    buffer="start";
    writeData(playerDescriptors[playerNumber], buffer);
    writeData(playerDescriptors[playerNumber],to_string(Game::getMapWidth()));
    writeData(playerDescriptors[playerNumber],to_string(Game::getMapHeight()));
    writeData(playerDescriptors[playerNumber],to_string(Game::getMaxPlayersNumber()));
    sleep(1);
    sendGameMap(playerDescriptors[playerNumber]);
    //send players positions
    Pair playerPosition = getPlayerPosition(playerDescriptors[playerNumber]);
    sendMoveToAll(playerDescriptors[playerNumber],playerPosition,playerPosition);
    sleep(1);

    while(!threadStop[playerNumber])
    {
        if(readData(playerDescriptors[playerNumber], buffer)==0)
        {
            endSignal();
        }
        if(buffer=="left" || buffer=="right" || buffer=="up" || buffer=="down")
        {
            makeMove(playerDescriptors[playerNumber],buffer);
            buffer="null";
        }
        else if(buffer=="bomb")
        {
            placeBomb(playerNumber);
        }
    }
}

Game::Game(int descriptors[]) : gameMap(mapWidth, vector<int>(mapHeight)),
    players(mapWidth, vector<int>(mapHeight)),
    playerDescriptors(maxPlayersNumber),
    threadStop(maxPlayersNumber),
    t(maxPlayersNumber),
    descriptorsMutex(maxPlayersNumber),
    bombs(maxPlayersNumber),
    bombThreads(3*maxPlayersNumber)
{
    loadMap();
    initPlayers();
    playersCount=maxPlayersNumber;
    for(int i=0; i<maxPlayersNumber; i++)
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
    for(int i=0; i<maxPlayersNumber; i++)
    {
        t[i] = thread(&Game::clientThread,this,i);
    }
}

Game::~Game()
{
    cout<<"end of game\n";
    for(int i=0; i<maxPlayersNumber; i++)
    {
        threadStop[i]=true;
        writeData(playerDescriptors[i],"end");
        if(t[i].joinable())
        {
            t[i].join();
        }
        close(playerDescriptors[i]);
    }
    sleep(4);
}

void Game::loadConfig()
{
    cout<<"config="<<configFile<<endl;
    ifstream file(configFile);
    if(file.is_open())
    {
        int i = 0;
        string line;
        while(getline(file,line))
        {
            size_t pos = line.find("=");
            if(pos!=string::npos)
            {
                string name = line.substr(0,pos);
                string strValue = line.substr(pos+1);
                int value = stoi(strValue);
                if(name=="maxPlayersNumber")
                    Game::maxPlayersNumber=value;
                else if(name=="mapWidth")
                    Game::mapWidth=value;
                else if(name=="mapHeight")
                    Game::mapHeight=value;
                else if(name=="serverIp")
                {
                    Game::serverIp=strValue;
                }
            }
            else
                cout<<"Invalid line "<<i<<" in config file\n";
            i++;
        }
        file.close();
        cout<<"loaded "<<i<<"lines from config file\n";
    }
    else
        cout<<"Failed to load config\n"<<
            "Starting with default settings\n";
}

void Game::loadMap()
{
    ifstream file(mapFile);
    string line;
    if(file.is_open())
    {
        for(int i=0; i<mapHeight; i++)
        {
            getline(file,line);
            for(int j=0; j<mapWidth; j++)
            {
                Game::gameMap[i][j]=line[j] - '0';
            }
        }
    }
}

string Game::getExecutablePath()
{
    string fullFileName = "";

    // Code taken from: http://www.gamedev.net/community/forums/topic.asp?topic_id=459511
    std::string path = "";
    pid_t pid = getpid();
    char buf[20] = {0};
    sprintf(buf,"%d",pid);
    std::string _link = "/proc/";
    _link.append( buf );
    _link.append( "/exe");
    char proc[512];
    int ch = readlink(_link.c_str(),proc,512);
    if (ch != -1) {
        proc[ch] = 0;
        path = proc;
        std::string::size_type t = path.find_last_of("/");
        path = path.substr(0,t);
    }

    fullFileName = path + string("/");
    return fullFileName;
}
