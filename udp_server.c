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
#include <sys/stat.h>
/* You will have to modify the program below */

#define MAXBUFSIZE 100
#define SENDBUF_SIZE 2048
#define RECVBUF_SIZE 2048


void start_service();

int get_file_size(char *fd);
char * get_file_name(char * recvbuf);
void delete_file(char * filename);



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

    int nbytes, file_size;
    FILE *fp;

    while(1){
        //Clear the receive buffer
        bzero(recvbuf,RECVBUF_SIZE);
        //waits for an incoming message
    	nbytes = recvfrom(sock,recvbuf,RECVBUF_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
        printf("Command Received: %s\n",recvbuf);

        if(nbytes!= 0){
            /**** ls ****/
            if(!strncmp(recvbuf,"ls",nbytes)){
                printf("Sending filenames in directory\n");
                system("ls > filenames.txt");

                char *filename = "filenames.txt";
                fp = fopen(filename,"r");
                if(fp==NULL){
                    printf("file does not exist\n");
                }
                file_size = get_file_size(filename);
                if(fread(sendbuf,file_size,1,fp)<=0)
                {
                  printf("unable to copy file into buffer\n");
                  exit(1);
                }
                nbytes = file_size;
                fclose(fp);
            }

            /**** get file ****/
            else if (!strncmp(recvbuf,"get ",4)){
                printf("Sending file\n");
                nbytes = 0;
                char * filename = get_file_name(recvbuf);
                //Handle if no filename after get
            }

            /**** put file ****/
            else if (!strncmp(recvbuf,"put ",4)){
                printf("Receiving file\n");
                // Prepare to receive file

            }

            /**** delete file ****/
            else if (!strncmp(recvbuf,"delete ",7)){
                //Handle if no filename after delete
                char * filename = get_file_name(recvbuf);
                if(filename == NULL){
                    strcat(sendbuf,"File does not exist");
                    nbytes = strlen(sendbuf);
                }
                else{
                    printf("Deleting file %s\n",filename);
                    delete_file(filename);
                    nbytes = 0;
                }
            }

            /**** Close socket and exit ****/
            else if(!strncmp(recvbuf,"exit",nbytes)){
                nbytes = sendto(sock,"exit",4,0,(struct sockaddr *) &sock_addr, addrlen);
                close(sock);
                exit(0);
            }
            /**** Command not found, ECHO ****/
            else{
                strncpy(sendbuf,recvbuf,nbytes); //Echo the command back to the client unmodified
            }
        }

        nbytes = sendto(sock,sendbuf,nbytes,0,(struct sockaddr *) &sock_addr, addrlen);
        bzero(sendbuf,SENDBUF_SIZE);
    }

    exit(0);
}


/* Get the file size in bytes */
int get_file_size(char *filename){

    struct stat buf;
    stat(filename, &buf);
    int size = buf.st_size;
    return size;
}

/* Find the space character and return string after the space character*/
char * get_file_name(char * recvbuf){

    char* filename = memchr(recvbuf,32,7);
    filename += 1;  //Increment by 1 to avoid the space character
    printf("Filename is: %s\n",filename);
    FILE *fp;
    fp = fopen(filename,"r");
    if(fp == NULL ){
        printf("File does not exist\n");
        return NULL;
    }
    fclose(fp);
    return filename;
}



void delete_file(char * filename){

    char command[256];
    bzero(command,sizeof(command));
    strcat(command, "rm -f ");
    strcat(command, filename);
    system(command);
}
