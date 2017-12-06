Distributed File Server

A Distributed File System is a client/server-based application that allows
client to store and retrieve files on multiple servers. One of the features of
Distributed file system is that each file can be divided in to pieces and stored
on different servers and can be retrieved even if one server is not active.

This project submission contains two .c files that can be compiled to run a TCP
distributed fileserver using the makefile.

---dfs.c--- Once compiled an executable server is created. The arguments passed are:
1. DFS directory
2. Port number

To start the server first run:
$make server   #then,
$./dfs /DFS1 10001
$./dfs /DFS2 10002
$./dfs /DFS3 10003
$./dfs /DFS4 10004

 
---dfc.c--- Once compiled an executable client is created. The arguments passed are:
1. Configuration file

To start the client run:
$make client    #then,
$./dfc dfc.conf


Functions Implemented:
1. list - This function lists all the files that are present at the server side

2. get - This function retrieves the file as required from the servers and
        recombines the pieces to form the complete file

3. put - This function breaks the files into multiple pieces and then stores
        these pieces in the multiple file servers

4. exit - This function exits all the client and server instances gracefully
