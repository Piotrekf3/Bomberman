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
#include <SFML/Graphics.hpp>
using namespace std;

const char * ip = "127.0.0.1";
const int mapWidth = 10;
const int mapHeight = 10;
int gameMap[mapWidth][mapHeight];
int players[mapWidth][mapHeight];

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
	char keyPressed[10]="null";
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
			if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
			{
				cout<<"left"<<endl;
				strcpy(keyPressed,"left");
			}
			else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
			{
				cout<<"right"<<endl;
				strcpy(keyPressed,"right");
			}
			if(strcmp(keyPressed,"null")!=0)
			{
				writeData(sd,keyPressed,strlen(keyPressed)+1);
				cout<<sizeof(keyPressed);
				strcpy(keyPressed,"null");
			}
            if (event.type == sf::Event::Closed)
                window.close();
        }
		int rectangleWidth = window.getSize().x/mapWidth;
		int rectangleHeight = window.getSize().y/mapHeight;

        window.clear();
		for(int i=0;i<mapWidth;i++)
			for(int j=0;j<mapHeight;j++)
			{
				rectangles[i][j].setSize(sf::Vector2f(rectangleWidth,rectangleHeight));
				rectangles[i][j].setPosition(sf::Vector2f(i*rectangleWidth,j*rectangleHeight));
				if(gameMap[i][j]==0)
				{
					rectangles[i][j].setFillColor(sf::Color(0,255,0));
				}
				else if(gameMap[i][j]==1)
				{
					rectangles[i][j].setFillColor(sf::Color(255,0,0));
				}
				window.draw(rectangles[i][j]);
			}
        window.display();
    }
}

void clientRead(int sd)
{
    ssize_t bufsize = 255, received;
    char buffer[bufsize];
	cout<<"Oczekiwanie na pozostałych graczy\n";
	while(strcmp(buffer,"start")!=0)
    readData(sd, buffer, bufsize);
    cout<<"start gry\n";
	thread window = thread(sfmlWindow,sd);
    while(1)
    {
        received = readData(sd, buffer, bufsize);
        writeData(1, buffer, received);
    }

}

void clientWrite(int sd)
{
        ssize_t bufsize = 255, received;
        char buffer[bufsize];
        received = readData(0, buffer, bufsize);
        writeData(sd, buffer, received);
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
