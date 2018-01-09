#include <iostream>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <cstdlib>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <algorithm>
#include <mutex>
#include <signal.h>
#include <SFML/Graphics.hpp>
using namespace std;

string ip = "127.0.0.1";
int maxPlayersNumber=2;
int mapWidth = 10;
int mapHeight = 10;
vector<vector<int>> gameMap;
vector<vector<int>> players;
mutex readMutex;
bool endRead = false;

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
    Pair(string x, string y)
    {
        this->x = stoi(x);
        this->y = stoi(y);
    }
};

void printPlayers()
{
    for(int i=0; i<mapWidth; i++)
    {
        for(int j=0; j<mapHeight; j++)
            cout<<players[i][j];
        cout<<endl;
    }
}

ssize_t readData(int fd, string& buffer) {
    char cbuffer;
    buffer="";
    ssize_t ret=1;
    while(ret)
    {
        ret=read(fd,&cbuffer,1);
        if(ret==0)
        {
            cout<<"Connection to server lost\n";
            exit(0);
        }
        if(cbuffer=='!')
            break;
        else
            buffer+=string(1,cbuffer);
    }
    cout<<buffer<<endl;
    return buffer.length();
}

ssize_t writeData(int fd, const string& buffer) {
    ssize_t ret = send(fd, (buffer+"!").c_str(), buffer.length()+1,MSG_NOSIGNAL);
    if(ret==-1) error(1, errno, "write failed on descriptor %d", fd);
    return ret;
}

void sfmlWindow(int sd)
{
    sf::RenderWindow window(sf::VideoMode(500,500),"Bomberman");
    sf::View view(sf::Vector2f(250,250), sf::Vector2f(500,500));
    window.setView(view);
    float rectangleWidth = float(window.getSize().x)/mapWidth;
    float rectangleHeight = float(window.getSize().y)/mapHeight;

    vector<vector<sf::RectangleShape>> rectangles(mapWidth,vector<sf::RectangleShape>(mapHeight));
    for(int i=0; i<mapWidth; i++)
        for(int j=0; j<mapWidth; j++)
        {
            rectangles[i][j].setSize(sf::Vector2f(rectangleWidth,rectangleHeight));
            rectangles[i][j].setPosition(sf::Vector2f(j*rectangleHeight,i*rectangleWidth));
        }

    vector<sf::CircleShape> bombs(maxPlayersNumber*3);
    for(int i=0; i<maxPlayersNumber*3; i++)
	{
        bombs[i].setFillColor(sf::Color(0,0,0));
		bombs[i].setRadius(rectangleWidth/4);
	}

    sf::CircleShape playerCircles[maxPlayersNumber];
	for(int i=0;i<maxPlayersNumber;i++)
		playerCircles[i].setRadius(rectangleWidth/2);

    string keyPressed="null";
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            {
                keyPressed="left";
            }
            else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            {
                keyPressed="right";
            }
            else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            {
                keyPressed="down";
            }
            else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            {
                keyPressed="up";
            }
            else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
            {
                keyPressed="bomb";
            }
            if(keyPressed!="null")
            {
                writeData(sd,keyPressed);
                cout<<"Pressed="<<keyPressed<<endl;
                keyPressed="null";
            }
            if (event.type == sf::Event::Closed)
                window.close();
        }
        int player=0;
        int bomb=0;

        lock_guard<mutex> lock(readMutex);
        window.clear();
        for(int i=0; i<mapWidth; i++)
            for(int j=0; j<mapHeight; j++)
            {
                if(gameMap[i][j]==0)
                {
                    rectangles[i][j].setFillColor(sf::Color(0,255,0));
                }
                else if(gameMap[i][j]==1)
                {
                    rectangles[i][j].setFillColor(sf::Color(255,165,0));
                }
                else if(gameMap[i][j]==2)
                {
                    rectangles[i][j].setFillColor(sf::Color(255,0,0));
                }
                window.draw(rectangles[i][j]);
                if(gameMap[i][j]==3 && bomb<maxPlayersNumber*3)
                {
                    bombs[bomb].setPosition(sf::Vector2f(j*rectangleHeight+bombs[bomb].getRadius(),
                                                         i*rectangleWidth+bombs[bomb].getRadius()));
                    window.draw(bombs[bomb]);
                    bomb++;
                }
                if(players[i][j]!=0 && player<maxPlayersNumber)
                {
                    playerCircles[player].setFillColor(sf::Color(255,255,0));
                    playerCircles[player].setPosition(sf::Vector2f(j*rectangleHeight,i*rectangleWidth));
                    window.draw(playerCircles[player]);
                    player++;
                }
            }
        window.display();
    }
}

void makeMove(int player,Pair from,Pair to)
{
    players[from.x][from.y]=0;
    players[to.x][to.y]=player;
}

void changeMap(Pair where, int value)
{
    gameMap[where.x][where.y]=value;
}

void killPlayer(Pair where)
{
    players[where.x][where.y]=0;
}

void startGame(int sd)
{
    //ssize_t bufsize = 255;
    string buffer;
    cout<<"Waiting for other players\n";
    while(buffer!="start")
    {
        readData(sd, buffer);
        cout<<buffer<<endl;
        writeData(sd,"ready");
    }
    readData(sd, buffer);
    mapWidth=stoi(buffer);
    readData(sd, buffer);
    mapHeight=stoi(buffer);
    cout<<mapWidth<<endl;
    cout<<mapHeight<<endl;

    gameMap.resize(mapWidth,vector<int>(mapHeight));
    players.resize(mapWidth,vector<int>(mapHeight));

    readData(sd, buffer);
    maxPlayersNumber=stoi(buffer);
    cout<<maxPlayersNumber<<endl;

    cout<<"start gry\n";
}

void clientRead(int sd)
{
    string message;
    while(endRead==false)
    {
        readData(sd, message);
        lock_guard<mutex> lock(readMutex);
        if(message=="end")
        {
            cout<<"koniec gry\n";
            writeData(sd,"stop");
            close(sd);
            exit(0);
        }
        //cout<<message<<endl;
        //ruch
        if(count(message.begin(),message.end(), ';')==4)
        {
            string delimiter = ";";
            size_t pos = 0;
            string token[5];
            int i=0;
            while ((pos = message.find(delimiter)) != string::npos)
            {
                token[i] = message.substr(0, pos);
                i++;
                message.erase(0, pos + delimiter.length());
            }
            if(token[3]=="-1" && message=="-1")
                killPlayer(Pair(token[1],token[2]));
            else
                makeMove(stoi(token[0]),Pair(token[1],token[2]),Pair(token[3],message));
        }
        //zmiana na mapie
        else if(count(message.begin(),message.end(), ';')==2)
        {
            string delimiter = ";";
            size_t pos = 0;
            string token[3];
            int i=0;
            while ((pos = message.find(delimiter)) != string::npos)
            {
                token[i] = message.substr(0, pos);
                i++;
                message.erase(0, pos + delimiter.length());
            }
            changeMap(Pair(token[0],token[1]),stoi(message));
        }
    }

}

bool checkIp(string ip)
{
    if(count(ip.begin(),ip.end(),'.')==3)
        return true;
    else return false;
}
//not used
void intHandler(int sd)
{
    endRead=true;
    close(sd);
    exit(0);
}

int main(int args, char * argv[]) {
    if(args>1 && checkIp(argv[1]))
    {
        cout<<"connecting to server on "<<argv[1]<<endl;
        ip=argv[1];
    }
    else
        cout<<"connecting to server on localhost\n";
    int sd = socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(2500);
    saddr.sin_addr.s_addr = inet_addr(ip.c_str());

    int cd = connect(sd,(sockaddr*) &saddr,sizeof(saddr));
    cout<<"cd="<<cd<<endl;
    if(cd==0)
    {
        startGame(sd);
        thread t(clientRead,sd);
        sfmlWindow(sd);
        t.join();
    }
    return 0;
}
