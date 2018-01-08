#Bomberman client makefile

BUILDPATH=./build
OBJPATH=./obj
CC=g++
CFLAGS= -g -O3 -c -Wall -pthread -std=c++11
LDFLAGS= -pthread
FILENAME=server

all: getobj exe putobj

exe: main.o Game.o
	$(CC) $(LDFLAGS) -o $(BUILDPATH)/$(FILENAME) main.o Game.o

main.o: main.cpp 
	$(CC) $(CFLAGS) main.cpp

getobj:
	-@mv $(OBJPATH)/*.o . 2>/dev/null
putobj:
	-@mv *.o $(OBJPATH) 2>/dev/null
Game.o: Game.cpp Game.h
	$(CC) $(CFLAGS) Game.cpp  
run:
	$(BUILDPATH)/$(FILENAME)
init:
	mkdir $(OBJPATH)
	mkdir $(BUILDPATH)
clear:
	rm -f $(BUILDPATH)/$(FILENAME)
	rm -f $(OBJPATH)/*.o
