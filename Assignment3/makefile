CC = gcc
CFLAGS = -g -lcrypto -lssl

ALL: client server

client: clean_client
	$(CC) dfc.c $(CFLAGS) -o dfc

server:  clean_server
	$(CC) dfs.c $(CFLAGS) -o dfs

clean: clean_client clean_server

clean_client:
	rm -rf dfc

clean_server:
	rm -rf dfs
