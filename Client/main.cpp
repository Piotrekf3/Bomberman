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
const int maxPlayersNumber=2;
const int mapWidth = 10;
const int mapHeight = 10;
int gameMap[mapWidth][mapHeight];
int players[mapWidth][mapHeight];

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
	for(int i=0; i<mapWidth;i++)
	{
		for(int j=0;j<mapHeight;j++)
			cout<<players[i][j];
		cout<<endl;
	}
}

ssize_t readData(int fd, char * buffer, ssize_t buffsize) {
    auto ret = read(fd, buffer, buffsize);
    if(ret==-1) error(1,errno, "read failed on descriptor %d", fd);
    return ret;
}

void writeData(int fd, char * buffer, ssize_t count) {
    auto ret = write(fd, buffer, count);
    if(ret==-1) error(1, errno, "write failed on descriptor %d", fd);
    if(ret!=count) error(0, errno, "wrote less than requested to descriptor %d (%ld/%ld)", fd, count, ret);
}

void sfmlWindow(int sd)
{
    sf::RenderWindow window(sf::VideoMode(500,500),"Bomberman");
    sf::RectangleShape rectangles[mapWidth][mapHeight];
	sf::CircleShape playerCircles[maxPlayersNumber];
    char keyPressed[10]="null";
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            {
                strcpy(keyPressed,"left");
            }
            else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            {
                strcpy(keyPressed,"right");
            }
			else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
			{
				strcpy(keyPressed,"down");
			}
			else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
			{
				strcpy(keyPressed,"up");
			}
            if(strcmp(keyPressed,"null")!=0)
            {
                writeData(sd,keyPressed,strlen(keyPressed)+1);
				cout<<"Pressed="<<keyPressed<<endl;
                strcpy(keyPressed,"null");
            }
            if (event.type == sf::Event::Closed)
                window.close();
        }
        int rectangleWidth = window.getSize().x/mapWidth;
        int rectangleHeight = window.getSize().y/mapHeight;
		int player=0;

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
				if(players[i][j]!=0)
				{
//					cout<<"i="<<i<<"j="<<j<<endl;
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

void clientRead(int sd)
{
    ssize_t bufsize = 255;
    char buffer[bufsize];
    cout<<"Oczekiwanie na pozostałych graczy\n";
    while(strcmp(buffer,"start")!=0)
        readData(sd, buffer, bufsize);
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

void clientWrite(int sd)
{
    /*        ssize_t bufsize = 255, received;
            char buffer[bufsize];
            received = readData(0, buffer, bufsize);*/
}

void initGameMap()
{
    for(int i=0; i<mapHeight; i++)
        for(int j=0; j<mapWidth; j++)
            gameMap[i][j]=0;
}

int main() {
    int sd = socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(2500);
    saddr.sin_addr.s_addr = inet_addr(ip);

    initGameMap();
    thread read;
    thread write;
    int cd = connect(sd,(sockaddr*) &saddr,sizeof(saddr));
    if(cd==0)
    {
        read = thread(clientRead,sd);
//        write = thread(clientWrite,sd);
        read.join();
//        write.join();
    }

    return 0;
}
