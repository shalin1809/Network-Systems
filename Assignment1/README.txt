**UDP Socket Programming**

UDP(Use Datagram Protocol) is a connectionless transmission model with a minimum of protocol mechanism.
It has no handshaking dialogues and no mechanism of acknowledgements(ACKs). Therefore, every packet delivered
by using UDP protocol is not guaranteed to be received. That’s why UDP is an unreliability protocol.
Due to the simplicity, it avoids unnecessary delay in some specific application such as streaming media.

The project contains 2 directories:
The Client folder has the udp_client.c code file, makefile, foo1, foo2 and foo3 files.
The Server folder has the udp_server.c code file and makefile.

The file transfer between Server and Client is encrypted using XOR method and works reliably with remote server.

Start the server first:
    /Server$ make                  #compile the code
    /Server$ ./server_udp [port]   #Run the server with the required port number

On starting the server, the server waits for first handshake from client

Now start the client:
    /Client$ make                   #compile the code
    /Client$ ./client_udp [IP] [port]   #Run the client with the server IPv4 address and the required port number

On starting the client (with the server already running), you can see Server ping success on the client side and
Client ping success on the server side
The Client also shows the commands accepted and it is waiting for a command.
Here on, both the client and the server are in a blocking loop which can be exited using the menu command or CTRL+C
You can see the following output on the terminal of the client:

/client_udp 127.0.0.1 5000
Server ping success
--------------------------------------
Commands accepted
ls
get [file_name]
put [file_name]
delete[file_name]
exit
--------------------------------------
Waiting for command:


The following commands are implemented:
1. ls                   #Get the names of the files in the directory of the server
    The client sends the command ls to the server.
    The server performs a system command which generates a file "filenames.txt" containing the names of the file
    in the directory.
    The server now sends this file to the client which is printed on the client terminal.
    After sending this file, the server deletes the file.

    Usage on client terminal:
    --------------------------------------
    Waiting for command:
    ls
    Server: filenames.txt
    Makefile
    server_udp
    udp_server.c


2. get [file_name]      #Get the file "file_name" from the server
    The client sends this command to the server.
    The server checks if the file exists and then sends acknowledgement with the number of packets and filesize in bytes.
    If the file does not exist, the server conveys this to the client and both resume to the menu in the main loop.
    After receiving the number of packets and filesize, the client opens a new file and waits to receive the first packet
    The server starts a timeout using setsockopt() function call, encrypts the file and sends the first packet.
    Each transmitted packet has 1000 bytes of data from the file and a sequence number appended to identify the packet.
    On receiving the packet, the client extracts the sequence number and seeks to the corresponding address in the file
    and writes the data to that position.
    The client now sends and ACK with the sequence number of the packet it received.
    The server receives this ACK, extracts the packet number, seeks to the next packet and sends it.
    Thus the entire file is transferred this way. The terminals print log messages for every 100 packets sent.
    The server sends endoffile packet, denoting the end of data.
    The client decrypts the file, and checks if the received file size is same as the filesize sent initially.
    If the file sizes don't match, the client sends the get [file_name] command again.
    In case any packet or ACK is dropped, the recvfrom in the server will timeout and resend the previous packet.
    On finishing this, the client sends success which is echoed by the server

    Usage on client terminal:
    --------------------------------------
    Waiting for command:
    get foo2
    Received: File 2 1469
    Received packet: 0
    Received endoffile
    Received file size is: 1469
    Server: success

    --------------------------------------
    Waiting for command:
    get foo
    Server: File does not exist



3. put [file_name]      #Send the file "file_name" to the server
    The client sends this command to the server.
    The client checks if the file exists and then sends the number of packets and filesize in bytes.
    If the file does not exist, the client conveys this to the server and both resume to the menu in the main loop.
    On receiving the filesize and number of packets, the server tells the client that it is ready to receive the file
    The server opens a new file and waits to receive the first packet.
    The client starts a timeout using setsockopt() function call, encrypts the file and sends the first packet.
    Each transmitted packet has 1000 bytes of data from the file and a sequence number appended to identify the packet.
    On receiving the packet, the server extracts the sequence number and seeks to the corresponding address in the file
    and writes the data to that position.
    The server now sends and ACK with the sequence number of the packet it received.
    The client receives this ACK, extracts the packet number, seeks to the next packet and sends it.
    Thus the entire file is transferred this way. The terminals print log messages for every 100 packets sent.
    The client sends endoffile packet, denoting the end of data.
    The server decrypts the file, and checks if the received file size is same as the filesize sent initially.
    If the file sizes don't match, the server sends put failed to the client.
    In case any packet or ACK is dropped, the recvfrom in the client will timeout and resend the previous packet.

    Usage on client terminal:
    --------------------------------------
    Waiting for command:
    put foo2
    File ready: Sending File 2 1469 19
    Server ready
    Sending packet: 0
    ACK packet: 0
    Sending packet: 1
    Reached endoffile
    Server: success

    --------------------------------------
    Waiting for command:
    put foo
    File does not exist
    Server: put failed


4. delete [file_name]   #Deletes the file "file_name" in the server directory
    The client sends this command to the server.
    The server first checks if the file exists.
    It deletes the file and send success on deleting the file.
    If the file does not exist, it tell the client that the file does not exist

    Usage on client terminal:
    --------------------------------------
    Waiting for command:
    delete foo2
    Server: success

    --------------------------------------
    Waiting for command:
    delete foo
    Server: File does not exist



5. exit                 #Close the socket and exit the program
    The client sends the exit command to the server.
    The server replies the client to exit, closes the socket and exits the program.
    The client on receiving exit closes the socket and exits the program.

    Usage on client terminal:
    --------------------------------------
    Waiting for command:
    exit



5. Unknown command      #Echo
    The server does not recognise this command and simply echoes it to the client without any modification

    Usage on client terminal:
    --------------------------------------
    Waiting for command:
    hello
    Server: hello


The file encryption is done by XORing "SHALIN" to every 6 bytes of the file and saving it to a new temp file.
In case of encryption, the file pointer to be sent is made to point to the temp file. Thus the original file stays
unmodified and the temp file is transferred. This temp file is deleted on file transfer completion.
In case of decryption, the original file is deleted and the temp file is renamed to the original file.
