#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
/* You will have to modify the program below */

#define MAXBUFSIZE 100
#define SENDBUF_SIZE 1024
#define RECVBUF_SIZE 1024


void start_service();


int main (int argc, char * argv[] )
{
	int sock;                           //This will be our socket
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
    char sendbuffer[SENDBUF_SIZE], recvbuffer[RECVBUF_SIZE]; //Buffer for sending and receiving data

	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine


	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0)
	{
		printf("unable to create socket");
	}

	/******************
	  Once we've created a socket, we must bind that socket to the
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
	}

	remote_length = (socklen_t)sizeof(remote);

    //Empty the buffer
    bzero(buffer,sizeof(buffer));

    //waits for an incoming message
	nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *) &sin, &remote_length);

	printf("Client ping %s\n", buffer);

	char msg[] = "success";
	nbytes = sendto(sock,msg,sizeof(msg),0,(struct sockaddr *) &sin, remote_length);

    start_service(sock,sendbuffer,recvbuffer,sin,remote_length);

	close(sock);
}



void start_service(int sock, char *sendbuf, char *recvbuf, struct sockaddr_in sock_addr, socklen_t addrlen){


    int nbytes;
    while(1){
        bzero(recvbuf,RECVBUF_SIZE);
        //waits for an incoming message
    	nbytes = recvfrom(sock,recvbuf,RECVBUF_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
        printf("Command Received: %s\n",recvbuf);

        //Exit command received
        if(!strncmp(recvbuf,"exit",nbytes)){
            nbytes = sendto(sock,"exit",4,0,(struct sockaddr *) &sock_addr, addrlen);
            close(sock);
            exit(0);
        }
        //Command not found
        else{
            nbytes = sendto(sock,recvbuf,nbytes,0,(struct sockaddr *) &sock_addr, addrlen);
        }

    }

    exit(0);

}
