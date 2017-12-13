#Bomberman client makefile

BUILDPATH=./build
OBJPATH=./obj
CC=g++
CFLAGS=-g -Wall -pthread -std=c++11
SFMLFLAGS=-lsfml-graphics -lsfml-window -lsfml-system
FILENAME=client

all: main

main: 
	@echo "***** compilation started *****"
	$(CC) $(CFLAGS) -o $(BUILDPATH)/$(FILENAME) main.cpp $(SFMLFLAGS)

getobj:
	-@mv $(OBJPATH)/*.o . 2>/dev/null
putobj:
	-@mv *.o $(OBJPATH) 2>/dev/null
run:
	$(BUILDPATH)/$(FILENAME)
init:
	mkdir $(OBJPATH)
	mkdir $(BUILDPATH)
clear:
	rm -f $(BUILDPATH)/$(FILENAME)
	rm -f $(OBJPATH)/*.o
