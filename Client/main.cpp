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
#include <SFML/Graphics.hpp>
using namespace std;

const char * ip = "127.0.0.1";
int maxPlayersNumber=2;
int mapWidth = 10;
int mapHeight = 10;
vector<vector<int>> gameMap;
vector<vector<int>> players;

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

ssize_t readData(int fd, string& buffer, ssize_t buffsize) {
    char cbuffer[buffsize];
    auto ret = read(fd, cbuffer, buffsize);
    if(ret==-1) error(1,errno, "read failed on descriptor %d", fd);
    buffer=cbuffer;
    return ret;
}

void writeData(int fd, const string& buffer, ssize_t count) {
    auto ret = write(fd, buffer.c_str(), count);
    if(ret==-1) error(1, errno, "write failed on descriptor %d", fd);
    if(ret!=count) error(0, errno, "wrote less than requested to descriptor %d (%ld/%ld)", fd, count, ret);
}

void sfmlWindow(int sd)
{
    sf::RenderWindow window(sf::VideoMode(500,500),"Bomberman");
    vector<vector<sf::RectangleShape>> rectangles(mapWidth,vector<sf::RectangleShape>(mapHeight));
	vector<sf::CircleShape> bombs(maxPlayersNumber*3);
	for(int i=0;i<maxPlayersNumber*3;i++)
		bombs[i].setFillColor(sf::Color(0,0,0));
    sf::CircleShape playerCircles[maxPlayersNumber];

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
                writeData(sd,keyPressed,255);
                cout<<"Pressed="<<keyPressed<<endl;
                keyPressed="null";
            }
            if (event.type == sf::Event::Closed)
                window.close();
        }
        int rectangleWidth = window.getSize().x/mapWidth;
        int rectangleHeight = window.getSize().y/mapHeight;
        int player=0;
		int bomb=0;

        window.clear();
        for(int i=0; i<mapWidth; i++)
            for(int j=0; j<mapHeight; j++)
            {
                rectangles[i][j].setSize(sf::Vector2f(rectangleWidth,rectangleHeight));
                rectangles[i][j].setPosition(sf::Vector2f(j*rectangleHeight,i*rectangleWidth));
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
					bombs[bomb].setRadius(rectangleWidth/4);
					bombs[bomb].setPosition(sf::Vector2f(j*rectangleHeight+bombs[bomb].getRadius(),
								i*rectangleWidth+bombs[bomb].getRadius()));
					window.draw(bombs[bomb]);
					bomb++;
				}
                if(players[i][j]!=0 && player<maxPlayersNumber)
                {
                    playerCircles[player].setRadius(rectangleWidth/2);
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

void clientRead(int sd)
{
    ssize_t bufsize = 255;
    string buffer;
    cout<<"Oczekiwanie na pozostałych graczy\n";
    while(buffer!="start")
        readData(sd, buffer, bufsize);

    readData(sd, buffer, bufsize);
    mapWidth=stoi(buffer);
    readData(sd, buffer, bufsize);
    mapHeight=stoi(buffer);
    cout<<mapWidth<<endl;
    cout<<mapHeight<<endl;

    gameMap.resize(mapWidth,vector<int>(mapHeight));
    players.resize(mapWidth,vector<int>(mapHeight));

    readData(sd, buffer, bufsize);
    maxPlayersNumber=stoi(buffer);
    cout<<maxPlayersNumber<<endl;

    cout<<"start gry\n";
    thread window = thread(sfmlWindow,sd);
    string message;
    while(1)
    {
        readData(sd, buffer, bufsize);
        message=buffer;
        cout<<message<<endl;
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
        //zmiana mapy
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

int main() {
    int sd = socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(2500);
    saddr.sin_addr.s_addr = inet_addr(ip);

    thread read;
    thread write;
    int cd = connect(sd,(sockaddr*) &saddr,sizeof(saddr));
    if(cd==0)
    {
        read = thread(clientRead,sd);
        read.join();
    }

    return 0;
}
