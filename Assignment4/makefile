CC = gcc
CFLAGS = -g -Wall -lcrypto

ALL: webproxy

webproxy: clean
	$(CC) webproxy.c $(CFLAGS) -o webproxy

clean:
	rm -rf webproxy
