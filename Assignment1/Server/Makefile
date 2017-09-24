#Makefile
CC = gcc
INCLUDE = /usr/lib
LIBS =
OBJS =
CFLAGS = -g

all: udp_server

udp_server:
	$(CC) -o server_udp udp_server.c $(CFLAGS) $(LIBS)

clean:
	rm -f server_udp
