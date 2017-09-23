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
#define RECVBUF_SIZE 1000
#define RECVBUF_PACKET_SIZE 1024


void start_service();
int get_file_size(char *fd);
char * get_file_name(char * recvbuf);
void delete_file(char * filename);
char* crypt_file(char * filename,char* crypted);



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

    int nbytes, file_size, command_length;
    char ACK[256];
    char command[256];
    char fname[256],cryptedfname[256];
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
                strncpy(fname,recvbuf,256);
                char * filename = get_file_name(fname);
                printf("File name in get: %s\n",filename);
                if(filename == NULL){
                    //Send error message if file does not exist
                    strcat(sendbuf,"File does not exist");
                    nbytes = strlen(sendbuf);
                }
                else{//File exists
                    //Encrypt the file before sending, this will create a temp file
                    crypt_file(filename,cryptedfname);
                    //Select the encrypted file to be sent
                    filename = cryptedfname;
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
                    //On successfully sending, delete the temp file
                    remove(cryptedfname);
                    //The sendto at the end of while(1) will send endoffile
                }
            }

            /**** put file ****/
            else if (!strncmp(recvbuf,"put ",4)){
                //Extract the file name
                strcpy(fname,recvbuf+4);
                char * filename = fname;
                // Prepare to receive file
                printf("Receiving file: %s\n", filename);
                //Receive file details from the client
                bzero(recvbuf,RECVBUF_PACKET_SIZE);
                nbytes = recvfrom(sock,recvbuf,RECVBUF_PACKET_SIZE,0,(struct sockaddr *) &sock_addr, &addrlen);
                //Received sending file command
                if(!strncmp(recvbuf,"Sending File ",13)){
                    int no_of_packet, file_size, current_packet=0,packet=1;
                    char temp[20];
                    //Extract the number of packets and file size
                    sscanf(recvbuf,"%s %s %d %d",temp,temp,&no_of_packet,&file_size);
                    printf("Received: %s %d %d\n",temp,no_of_packet,file_size);
                    //Create a new file and open it with write access
                    fp = fopen(filename,"w+");
                    //Clear the receive buffer
                    strcat(sendbuf,"Server ready");
                    nbytes = strlen(sendbuf);
                    nbytes = sendto(sock,sendbuf,nbytes,0,(struct sockaddr *) &sock_addr, addrlen);
                    bzero(recvbuf,RECVBUF_PACKET_SIZE);
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
                        bzero(sendbuf,SENDBUF_PACKET_SIZE);
                        //if(no_of_packet - current_packet){
                            //If the same packet is received again, do not ACK again
                            if(packet != current_packet){
                                sprintf(sendbuf,"ACK %d",current_packet);
                                packet = current_packet;
                                nbytes = sendto(sock,sendbuf,SENDBUF_PACKET_SIZE,0,(struct sockaddr *) &sock_addr, addrlen);
                            }
                        //}
                    }
                    printf("Outside for packet: %d\n\n", current_packet);
                    printf("Outside for: %s\n\n", recvbuf);
                    if(!strncmp(recvbuf,"endoffile",9)){
eof:                    printf("Received endoffile\n");
                        //File transfer complete, close the file
                        fclose(fp);
                        printf("The file sizes are %d  %d\n",file_size,get_file_size(filename));
                        //Check the received file size
                        if((file_size) == get_file_size((char*)filename)){
                            //Send success if file size received is matches the initial file size sent by the server
                            strcpy(sendbuf,"success");
                            nbytes = strlen(sendbuf);
                            nbytes = sendto(sock,sendbuf,nbytes,0,(struct sockaddr *) &sock_addr, addrlen);
                            printf("Received file size is: %d\n", file_size);
                            //Decrypt the received file, it will generate a temp file
                            crypt_file(filename,cryptedfname);
                            //Remove the received encrypted file
                            remove(filename);
                            //Rename the decrypted file
                            rename(cryptedfname,filename);
                            goto skipsendto;
                        }
                        else {
                            //If file size is not same, delete the file and repeat the process
                            printf("File size is not same\n");
                            strcpy(sendbuf,"File size is not same");
                            nbytes = strlen(sendbuf);
                            //delete file and send file size not same error
                            system("rm -f received");
                            //nbytes = sendto(sock,command,command_length,0,(struct sockaddr *) &sock_addr, addrlen);
                        }
                    }

                }
                else{
                    printf("Error\n");
                    strcat(sendbuf,"put failed");
                    nbytes = strlen(sendbuf);
                }

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
skipsendto:        bzero(sendbuf,SENDBUF_SIZE);
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


/* Encrypt/Decrypt file using XOR */
char* crypt_file(char * filename, char * crypted){
    FILE *fp, *fd;
    int file_size = get_file_size(filename);
    char* crypted_temp = "crypted_temp";
    //Open source file for reading
    fp = fopen(filename,"r");
    if(fp == NULL ){
        printf("File does not exist\n");
        return NULL;
    }
    //Create a temporary file for storing the XORed data
    fd = fopen(crypted_temp,"w+");
    if(fp == NULL ){
        printf("File does not exist\n");
        return NULL;
    }

    //Current data byte
    char data_byte;
    char * key = "SHALIN";
    int keysize = strlen(key);
    int count = 0;
    //Loop through each byte of file for the file size
    for(count;count<file_size;count++){
        data_byte = fgetc(fp);
        fputc((data_byte ^ key[count%keysize]), fd);
    }
    fclose(fp);
    fclose(fd);
    //Set the name of the crypted file
    strcpy(crypted,crypted_temp);
}
