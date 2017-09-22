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
#include <sys/stat.h>

#define MAXBUFSIZE 100
#define SENDBUF_SIZE 2048
#define RECVBUF_SIZE 1000
#define RECVBUF_PACKET_SIZE 1024

void start_service();
int get_file_size();

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

    //Start the service
    start_service(sock,sendbuffer,recvbuffer,remote,remote_length);

	close(sock);
}



void start_service(int sock, char *sendbuf, char *recvbuf, struct sockaddr_in sock_addr, socklen_t addrlen){

    char command[256];
    int nbytes, command_length;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;   //Set timeout to 100ms

    printf("Commands accepted\nls\nget [file_name]\nput [file_name]\ndelete[file_name]");
    printf("\nexit\nWaiting for command:\n");
    while (1) {
        //Clear the command buffer
        memset(command,0,sizeof(command));

        //Command input
        gets(command);

repeat: command_length = strlen(command);
        //Send command to server
        nbytes = sendto(sock,command,command_length,0,(struct sockaddr *) &sock_addr, addrlen);
        //Clear the receive buffersps
        bzero(recvbuf,RECVBUF_SIZE);

        //If the command is to get  a file
        if(!strncmp(command,"get ",4)){
            //Get file name
            char *filename = (command + 4);
            FILE *fp;
            //Send the command to the server
            nbytes = recvfrom(sock,recvbuf,RECVBUF_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
            //If file does not exist received
            if(!strncmp(recvbuf,"File does not exist",19)){
                //Print what the server responded
                printf("Server: %s\n\n", recvbuf);
                continue;
            }
            else if(!strncmp(recvbuf,"Sending File ",13)){
                int no_of_packet, file_size, current_packet=0,packet=1;
                char temp[20];
                sscanf(recvbuf,"%s %s %d %d",temp,temp,&no_of_packet,&file_size);
                printf("Received: %s %d %d\n",temp,no_of_packet,file_size);
                fp = fopen("received","w+");
                bzero(recvbuf,RECVBUF_PACKET_SIZE);
                for(current_packet=0;current_packet<=no_of_packet;current_packet++){
                    nbytes = recvfrom(sock,recvbuf,RECVBUF_PACKET_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
                    sscanf(recvbuf,"%d",&current_packet);
                    fseek(fp,current_packet*RECVBUF_SIZE,SEEK_SET);
                    fwrite((recvbuf+24),(nbytes-24),1,fp);
                    if(!strncmp(recvbuf,"endoffile",9)){
                        goto eof;
                    }
                    if((current_packet%100)==0){
                        printf("Received packet: %d\n", current_packet);
                    }
                    bzero(recvbuf,RECVBUF_PACKET_SIZE);
                    bzero(sendbuf,SENDBUF_SIZE);
                    if(no_of_packet - current_packet){
                        if(packet != current_packet){
                            sprintf(sendbuf,"ACK %d",current_packet);
                            packet = current_packet;
                            nbytes = sendto(sock,sendbuf,SENDBUF_SIZE,0,(struct sockaddr *) &sock_addr, addrlen);
                        }
                    }
                    //printf("Received packet: %d\n", current_packet);
                }
                //nbytes = recvfrom(sock,recvbuf,RECVBUF_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
                printf("Outside for packet: %d\n\n", current_packet);
                printf("Outside for: %s\n\n", recvbuf);
                if(!strncmp(recvbuf,"endoffile",9)){
eof:                printf("Received endoffile\n");
                    fclose(fp);
                    printf("The file sizes are %d  %d\n",file_size,get_file_size("received"));
                    if((file_size) == get_file_size(fp)){
                        strcpy(command,"success");
                        printf("Received file size is: %d\n", file_size);
                        nbytes = sendto(sock,command,command_length,0,(struct sockaddr *) &sock_addr, addrlen);
                        bzero(recvbuf,RECVBUF_SIZE);
                        nbytes = recvfrom(sock,recvbuf,RECVBUF_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
                    }
                    else {
                        printf("File size is not same\n");
                        system("rm -f received");
                        goto repeat;
                    }
                }
            }
            // FILE *fp;
            // fp = fopen("received","w+");
            // if(fwrite(recvbuf,nbytes,1,fp)<0){
            //   printf("Cannot save file\n");
            //   exit(1);
            // }

        }
        else{
            //Receive reply from server
            nbytes = recvfrom(sock,recvbuf,RECVBUF_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
            if((!strncmp(recvbuf,"exit",nbytes))&&nbytes!=0){
                close(sock);
                exit(0);
            }
        }
        //Print what the server responded
        printf("Server: %s\n\n", recvbuf);
        bzero(recvbuf,RECVBUF_SIZE);
    }

}



/* Get the file size in bytes */
int get_file_size(char *filename){

    struct stat buf;
    stat(filename, &buf);
    int size = buf.st_size;
    return size;
}
