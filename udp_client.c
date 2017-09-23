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
#define SENDBUF_SIZE 1000
#define SENDBUF_PACKET_SIZE 1024
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
    int nbytes, command_length, file_size;
    char ACK[256];
    char fname[256];
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
        //Clear the receive buffer
        bzero(recvbuf,RECVBUF_SIZE);

        //If the command is to get  a file
        if(!strncmp(command,"get ",4)){
            //Get file name
            strcpy(fname,command+4);
            char *filename = fname;
            FILE *fp;
            //Receive response from the server
            nbytes = recvfrom(sock,recvbuf,RECVBUF_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
            //If file does not exist received
            if(!strncmp(recvbuf,"File does not exist",19)){
                //Print what the server responded
                printf("Server: %s\n\n", recvbuf);
                continue;
            }
            //The server is now sending the file. The data received also has the number of packets and the file size in it
            else if(!strncmp(recvbuf,"Sending File ",13)){
                int no_of_packet, file_size, current_packet=0,packet=1;
                char temp[20];
                //Extract the number of packets and file size
                sscanf(recvbuf,"%s %s %d %d",temp,temp,&no_of_packet,&file_size);
                printf("Received: %s %d %d\n",temp,no_of_packet,file_size);
                //Create a new file and open it with write access
                fp = fopen(filename,"w+");
                //Clear the receive buffer
                bzero(recvbuf,RECVBUF_PACKET_SIZE);
                //Start loop to receive packets
                for(current_packet=0;current_packet<=no_of_packet;current_packet++){
                    nbytes = recvfrom(sock,recvbuf,RECVBUF_PACKET_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
                    //On receivin, get the sequence number at the start of the packet
                    sscanf(recvbuf,"%d",&current_packet);
                    //Move the file pointer accordingly
                    fseek(fp,current_packet*RECVBUF_SIZE,SEEK_SET);
                    //Check if endoffile received, exit loop
                    if(!strncmp(recvbuf,"endoffile",9)){
                        goto eof;
                    }
                    //Write the received data onto the file
                    fwrite((recvbuf+24),(nbytes-24),1,fp);
                    //Print on terminal for every 100 packets to track progress
                    if((current_packet%100)==0){
                        printf("Received packet: %d\n", current_packet);
                    }
                    //Clear the send and receive buffers
                    bzero(recvbuf,RECVBUF_PACKET_SIZE);
                    bzero(sendbuf,SENDBUF_SIZE);
                    //if(no_of_packet - current_packet){
                        //If the same packet is received again, do not ACK again
                        if(packet != current_packet){
                            sprintf(sendbuf,"ACK %d",current_packet);
                            packet = current_packet;
                            nbytes = sendto(sock,sendbuf,SENDBUF_SIZE,0,(struct sockaddr *) &sock_addr, addrlen);
                        }
                    //}
                }
                //nbytes = recvfrom(sock,recvbuf,RECVBUF_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
                printf("Outside for packet: %d\n\n", current_packet);
                printf("Outside for: %s\n\n", recvbuf);
                if(!strncmp(recvbuf,"endoffile",9)){
eof:                printf("Received endoffile\n");
                    //File transfer complete, close the file
                    fclose(fp);
                    printf("The file sizes are %d  %d\n",file_size,get_file_size(filename));
                    //Check the received file size
                    if((file_size) == get_file_size(filename)){
                        //Send success if file size received is matches the initial file size sent by the server
                        strcpy(command,"success");
                        printf("Received file size is: %d\n", file_size);
                        nbytes = sendto(sock,command,command_length,0,(struct sockaddr *) &sock_addr, addrlen);
                        bzero(recvbuf,RECVBUF_SIZE);
                        nbytes = recvfrom(sock,recvbuf,RECVBUF_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
                    }
                    else {
                        //If file size is not same, delete the file and repeat the process
                        printf("File size is not same\n");
                        system("rm -f received");
                        goto repeat;
                    }
                }
            }
        }
        //If the command is to get  a file
        else if(!strncmp(command,"put ",4)){
            char *filename = (command + 4);
            FILE *fp;
            //Open file
            fp = fopen(filename,"r");
            if(fp==NULL){
                printf("File does not exist\n");
                bzero(sendbuf,SENDBUF_PACKET_SIZE);
                strcat(sendbuf,"File does not exist");
                nbytes = strlen(sendbuf);
            }
            else{//File exists

                //Get the file size to be sent
                file_size = get_file_size(filename);
                int packet, max_packets = file_size/SENDBUF_SIZE;
                //Tell the client about the number of packets and the file size
                sprintf(sendbuf,"Sending File %d %d",max_packets+1,file_size);
                nbytes = strlen(sendbuf);
                printf("File ready: %s %d\n", sendbuf, nbytes);
                nbytes = sendto(sock,sendbuf,nbytes,0,(struct sockaddr *) &sock_addr, addrlen);
                //Clear the send and the receive buffers
                bzero(sendbuf,SENDBUF_PACKET_SIZE);
                bzero(recvbuf,RECVBUF_PACKET_SIZE);
                //wait for receivung ready from the server
                nbytes = recvfrom(sock,recvbuf,RECVBUF_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
                //Server Ready
                printf("%s\n",recvbuf);
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
                    nbytes = recvfrom(sock,recvbuf,RECVBUF_PACKET_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
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
                nbytes = sendto(sock,sendbuf,nbytes,0,(struct sockaddr *) &sock_addr, addrlen);
                bzero(sendbuf,SENDBUF_SIZE);
                //Close the opened file
                fclose(fp);
                //Clear receive buffer and wait for success
                bzero(recvbuf,RECVBUF_SIZE);
                nbytes = recvfrom(sock,recvbuf,RECVBUF_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
                //Reset the receive timeout so recvfrom becomes blocking again
                timeout.tv_sec = 0;
                timeout.tv_usec = 0;
                if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&timeout,sizeof(timeout)) < 0) {
                    perror("Error");
                }
                if(!strncmp(recvbuf,"File size is not same",21)){
                    goto repeat;
                }
            }
        }
        else{
            //Receive reply from server
            nbytes = recvfrom(sock,recvbuf,RECVBUF_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
            //If reply is exit, close socket and exit the program
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
