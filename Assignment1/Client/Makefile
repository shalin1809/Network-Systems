#Makefile
CC = gcc
INCLUDE = /usr/lib
LIBS =
OBJS =
CFLAGS = -g

all: udp_client 

udp_client:
	$(CC) -o client_udp udp_client.c $(CFLAGS) $(LIBS)


clean:
	rm -f client_udp
