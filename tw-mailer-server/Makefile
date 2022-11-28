#Makefile for TW-Mailer Server
#Authors: Max, Simon

all: server

server: main.cpp
	g++ -std=c++17 -Wall -Werror -o tw-mailer-server main.cpp -L/usr/lib/x86_64-linux-gnu -lldap -llber

clean:
	rm -f tw-mailer-server