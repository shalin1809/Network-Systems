#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#define MAXBUFSIZE 100
#define SENDBUF_SIZE 1024
#define RECVBUF_SIZE 1024

void start_service();

/* You will have to modify the program below */
int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];
    unsigned int remote_length;
	struct sockaddr_in remote;              //"Internet socket address structure"
    char sendbuffer[SENDBUF_SIZE], recvbuffer[RECVBUF_SIZE]; //Buffer for sending and receiving data

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/******************
	   Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet
	  i.e the Server.
	 ******************/
	bzero(&remote,sizeof(remote));               //zero the struct
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0)
	{
		printf("unable to create socket");
	}

    remote_length = (socklen_t)sizeof(remote);

	/******************
	  sendto() sends immediately.
	  it will report an error if the message   fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/
	char command[256] = "success";
	nbytes = sendto(sock,command,sizeof(command),0,(struct sockaddr *) &remote, remote_length);

	// Blocks till bytes are received
	struct sockaddr_in from_addr;
	int addr_length = sizeof(struct sockaddr);
	bzero(buffer,sizeof(buffer));
	nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *) &remote, &remote_length);
    printf("Server ping %s\n", buffer);


    start_service(sock,sendbuffer,recvbuffer,remote,remote_length);

	close(sock);
}



void start_service(int sock, char *sendbuf, char *recvbuf, struct sockaddr_in sock_addr, socklen_t addrlen){

    char command[256];
    int nbytes, command_length;
    printf("Commands accepted\nls\nget [file_name]\nput [file_name]\ndelete[file_name]");
    printf("\nexit\nWaiting for command:\n");
    while (1) {
        //Clear the command buffer
        memset(command,0,sizeof(command));

        //Command input
        gets(command);
        command_length = strlen(command);
        //Send command to server
        nbytes = sendto(sock,command,command_length,0,(struct sockaddr *) &sock_addr, addrlen);
        //Clear the receive buffer
        bzero(recvbuf,RECVBUF_SIZE);
        //Receive reply from server
        nbytes = recvfrom(sock,recvbuf,RECVBUF_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
        if((!strncmp(recvbuf,"exit",nbytes))&&nbytes!=0){
            close(sock);
            exit(0);
        }
        //Print what the server responded
        printf("Server: %s\n", recvbuf);
    }

}
