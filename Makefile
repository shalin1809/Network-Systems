#Makefile
CC = gcc
INCLUDE = /usr/lib
LIBS =
OBJS =

all: udp_client udp_server

udp_client:
	$(CC) -o client_udp -g udp_client.c $(CFLAGS) $(LIBS)
udp_server:
	$(CC) -o server_udp -g udp_server.c $(CFLAGS) $(LIBS)

clean:
	rm -f server_udp client_udp
