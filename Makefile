#Makefile for TW-Mailer Project
#Authors: Max, Simon

all: mailer

mailer: main.cpp
	gcc main.cpp -lstdc++ -Wall -o mailer

clean:
	rm -f mailer