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
#define SENDBUF_SIZE 1000
#define SENDBUF_PACKET_SIZE 1024
#define RECVBUF_SIZE 1024


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
    char sendbuffer[SENDBUF_PACKET_SIZE], recvbuffer[RECVBUF_SIZE]; //Buffer for sending and receiving data

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
    char ACK[256];
    FILE *fp;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;   //Set timeout to 100ms


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
                //Send a system command to generate a file for the files in directory
                system("ls > filenames.txt");
                //Open the generated file
                char *filename = "filenames.txt";
                fp = fopen(filename,"r");
                if(fp==NULL){
                    printf("file does not exist\n");
                }
                //Calculate the file size to be sent
                file_size = get_file_size(filename);
                //Read the file and copy it to sendbuf buffer
                if(fread(sendbuf,file_size,1,fp)<=0)
                {
                  printf("unable to copy file into buffer\n");
                  exit(1);
                }
                //Set the number of bytes to be sent as the file size
                nbytes = file_size;
                //Close the file
                fclose(fp);

                //The sendto function at the end of while(1) will send the file.
            }

            /**** get file ****/
            else if (!strncmp(recvbuf,"get ",4)){
                printf("Sending file\n");
                nbytes = 0;
                //Check if the file exists
                char * filename = get_file_name(recvbuf);
                if(filename == NULL){
                    //Send error message if file does not exist
                    strcat(sendbuf,"File does not exist");
                    nbytes = strlen(sendbuf);
                }
                else{//File exists
                    //Open file
                    fp = fopen(filename,"r");
                    if(fp==NULL){
                        printf("File does not exist\n");
                    }
                    //Get the file size to be sent
                    file_size = get_file_size(filename);
                    int packet, max_packets = file_size/SENDBUF_SIZE;
                    //Tell the client about the number of packets and the file size
                    sprintf(sendbuf,"Sending File %d %d",max_packets+1,file_size);
                    nbytes = strlen(sendbuf);
                    printf("File ready: %s %d\n", sendbuf, nbytes);
                    nbytes = sendto(sock,sendbuf,nbytes,0,(struct sockaddr *) &sock_addr, addrlen);
                    //Reset the send buffer
                    bzero(sendbuf,SENDBUF_PACKET_SIZE);
                    //Set timeout for the receive blocking
                    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&timeout,sizeof(timeout)) < 0) {
                        perror("Error");
                    }
                    //Start sending the file packet by packet, wait for ACK for each packet
                    for(packet = 0;packet<=max_packets;packet++){
                        //Append the packet number at the start of the packet
                        sprintf(sendbuf,"%d ",packet);
                        //Move the file pointer to the corresponding packet number
                        fseek(fp,packet*SENDBUF_SIZE,SEEK_SET);
                        //Load the file onto the send buffer
                        fread((sendbuf+24),SENDBUF_SIZE,1,fp);
                        //If it is the last packet to be sent, then calculate the remaining data size
                        if(max_packets - packet){
                            nbytes = SENDBUF_PACKET_SIZE;
                        }
                        else {
                            nbytes = (file_size + 24 - SENDBUF_SIZE*packet);
                            printf("Sending packet: %d\n",packet);
                        }
                        if(packet%100 == 0)
                            printf("Sending packet: %d\n", packet);
                        //Send the packet
                        nbytes = sendto(sock,sendbuf,nbytes,0,(struct sockaddr *) &sock_addr, addrlen);
                        //Reset the send buffer
                        bzero(sendbuf,SENDBUF_PACKET_SIZE);
                        //Reset the receive buffer
                        bzero(recvbuf,RECVBUF_SIZE);

                        //Wait to receive ACK from client, this will timeout
                        nbytes = recvfrom(sock,recvbuf,RECVBUF_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
                        //Check if ACK is received
                        if(!strncmp(recvbuf,"ACK ",4)){
                            //On receiveing ACK load the packet number for the ACK received
                            sscanf(recvbuf,"%s %d",ACK,&packet);
                            if(packet%100 == 0)
                                printf("ACK packet: %d\n", packet);
                        }
                            //If received anything other that ACK, then resend packet
                        else{
                            printf("Packet %d dropped\n",packet);
                            packet--;
                        }
                    }
                    printf("Reached endoffile\n");
                    //Load endoffile in send buffer for finishing file transfer
                    strcpy(sendbuf,"endoffile");
                    nbytes = 9;
                    //Close the opened file
                    fclose(fp);
                    //Reset the receive timeout so recvfrom becomes blocking again
                    timeout.tv_sec = 0;
                    timeout.tv_usec = 0;
                    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&timeout,sizeof(timeout)) < 0) {
                        perror("Error");
                    }
                    //The sendto at the end of while(1) will send endoffile
                }
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
                    //If file if available
                    printf("Deleting file %s\n",filename);
                    //Call delete file function which performs rm -f system call
                    delete_file(filename);
                    nbytes = 0;
                    //Send NULL using the sendto at the end of while(1)
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
    //Make a structure for the file properties
    struct stat buf;
    stat(filename, &buf);
    //Get the file size from the file properties
    int size = buf.st_size;
    return size;
}

/* Find the space character and return string after the space character*/
char * get_file_name(char * recvbuf){

    char* filename = memchr(recvbuf,32,7);
    filename += 1;  //Increment by 1 to avoid the space character
    printf("Filename is: %s\n",filename);
    FILE *fp;
    //Check if file exists
    fp = fopen(filename,"r");
    if(fp == NULL ){
        printf("File does not exist\n");
        return NULL;
    }
    fclose(fp);
    return filename;
}


/* Performs a system call to delete the filename received */
void delete_file(char * filename){

    char command[256];
    bzero(command,sizeof(command));
    strcat(command, "rm -f ");
    strcat(command, filename);
    system(command);
}
