#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdint.h>
#include<stdbool.h>
#include<openssl/md5.h>

#define CONNMAX 1000
#define BYTES 1024


char *ROOT;
char line[256];
int timeout;
int listenfd, clients[CONNMAX];
bool conf_missing=false;


void startServer(char *port);
void respond(int slot);
char * getvalue(char item[],char *file);
int get_file_size(char *filename);
int checkInFile(char item[],char *file);



int main(int argc, char * argv[]){

    struct sockaddr_in clientaddr;
	socklen_t addrlen;
    int slot=0;


    char PORT[6];
    char root[1000];

    if (argc < 3)
	{
		printf("USAGE: <server_port> <timeout>\n");
		exit(1);
	}

    //Get port from the arguments
    strcpy(PORT,argv[1]);

    //Get Root directory
    strcpy(root,getenv("PWD"));
    ROOT = root;

    //Get timeout from the arguments
    timeout = atoi(argv[2]);

    printf("Starting server at port no. %s, timeout interval %d with root directory as %s\n",PORT,timeout,ROOT);
    // Setting all elements to -1: signifies there is no client connected
    int i;
    for (i=0; i<CONNMAX; i++)
        clients[i]=-1;
    startServer(PORT);

    // ACCEPT connections
    while (1)
    {
        addrlen = sizeof(clientaddr);
        //Wait for new incoming connections
        clients[slot] = accept (listenfd, (struct sockaddr *) &clientaddr, &addrlen);

        if (clients[slot]<0)
            perror ("accept() error");
        else
        {
            //On accepting new requests, create a child process to send the response
            if ( fork()==0 )    //The parent process will wait for new connections
            {
                respond(slot);  //The child process responds and is killed after that
                exit(0);
            }
        }
        //Increment slot value and reuse slots
        while (clients[slot]!=-1) {
            slot = (slot+1)%CONNMAX;
        }
    }
    return 0;
}





//start server
void startServer(char *port){
	struct addrinfo hints, *res, *p;

	// getaddrinfo for host
	memset (&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo( NULL, port, &hints, &res) != 0)
	{
		perror ("getaddrinfo() error");
		exit(1);
	}
	// socket and bind
	for (p = res; p!=NULL; p=p->ai_next)
	{
		listenfd = socket (p->ai_family, p->ai_socktype, 0);
		if (listenfd == -1) continue;
		if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
	}
	if (p==NULL)
	{
		perror ("socket() or bind()");
		exit(1);
	}

	freeaddrinfo(res);

	// listen for incoming connections
	if ( listen (listenfd, 1000000) != 0 )
	{
		perror("listen() error");
		exit(1);
	}
}


//Client connection
void respond(int conn){

    char mesg[1500], copy[1500], *reqline[3];
    char data_to_send[1500], data_received[1500];
    char website[1000], temp[512];
    char * webptr, *req_ptr, *webaddr,*hash;
    char * sendbuf = data_to_send;
    char DNStemp[100],md[MD5_DIGEST_LENGTH],hashedASCII[32];
    int rcvd;
    int offset;
    struct sockaddr_in sockDNS;
    FILE *DNSfile, *SITEfile;

    //Reset the message buffer
	memset( (void*)mesg, (int)'\0', 1500);
    //receive the tcp data
	rcvd=recv(clients[conn], mesg, 1500, 0);
    strcpy(copy,mesg);


    if (rcvd<0)    // receive error
		fprintf(stderr,("recv() error\n"));
	else if (rcvd==0)    // receive socket closed
		fprintf(stderr,"Client disconnected upexpectedly.\n");
	else    // message received
	{
        //print the received message
		printf("\n\nReceived bytes %d and message:\n%s\n", rcvd, mesg);
        //Parse the received header
    	reqline[0] = strtok (mesg, " \t");
        //Check if it is a GET request
    	if ((strncmp(reqline[0], "GET\0", 4) == 0)){
            reqline[1] = strtok (NULL, " \t");      //File/website request
            printf("Website required: %s\n",reqline[1]);
			reqline[2] = strtok (NULL, " \t\n");    //HTTP version
            printf("HTTP version: %s\n",reqline[2]);


//TODO: Add HTTP version handling

            if((webptr = strstr(reqline[1],"www")) == NULL){    //If the website requested does not have www
                printf("Website does not contain www\n");
                if((webptr = strstr(reqline[1],"://")) == NULL){    //If the website requested does not have ://
                    printf("Website does not contain ://\n");
//TODO: Reply Not found error
                }
                else{   //If the website has :// after http, then set webptr to after ://
                    webptr+=3;  //3 for ://
                    strcpy(website,webptr);
                }
            }
            else{//If website has www, copy the address from www onwards
                strcpy(website,webptr);
            }
            *(strchr(website,'/')) = '\0';
            printf("Website: %s\n",website);

            //Check if website is blocked
            if(checkInFile(website,"BlockedList")){
siteblocked:    bzero(data_to_send,sizeof(data_to_send));
                //Prepare the send buffer with the bad request header
                offset = sprintf(sendbuf,"HTTP/1.1 403 Forbidden\n");
                sendbuf+=offset;
                //Append content type
                offset = sprintf(sendbuf,"Content-Type: .html\n");
                sendbuf+=offset;
                bzero(temp,sizeof(&temp));
                //Create the html file content
                strcpy(temp,"<html><body>ERROR 403 Forbidden");
                strcat(temp,"</body></html>");
                printf("Temp buffer: %s\n",temp);
                //Append content length
                offset = sprintf(sendbuf,"Content-Length: %d\n\n",(int)strlen(temp));
                sendbuf+=offset;
                //Append the html content
                strcpy(sendbuf,temp);
                //Print the data_to_send for debugging
                printf("data_to_send:\n%s\n",data_to_send);
                //Send the data to the client
                send(clients[conn], data_to_send, strlen(data_to_send),0);
                goto shut;
            }

            int soc;
            struct addrinfo hints, *servinfo, *p;
            int rv;

            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_INET;  //To force IPv4
            hints.ai_socktype = SOCK_STREAM;
            //Check if the IP exists in the DNS cache
            webaddr = getvalue(website,"DNScache");
            if(webaddr == NULL){

                //Lookup DNS to get the IP of the website
                if ((rv = getaddrinfo(website, "80", &hints, &servinfo)) != 0){
                    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                    exit(1);
                }

                // loop through all the results and connect to the first we can
                for(p = servinfo; p != NULL; p = p->ai_next) {
                    if ((soc = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
                        perror("socket");
                        continue;
                    }

                    if (connect(soc, p->ai_addr, p->ai_addrlen) == -1) {
                        perror("connect");
                        close(soc);
                        continue;
                    }
                    //Copy the socket structure
                    memcpy(&sockDNS,p->ai_addr,sizeof(sockDNS));
                    //Check if website is blocked
                    if(checkInFile(inet_ntoa(sockDNS.sin_addr),"BlockedList")){
                        close(soc);
                        goto siteblocked;
                        //iteblocked will send 403 Forbidden Error
                    }

                    //prepare a temp buffer with the website name and IP address
                    sprintf(DNStemp,"%s %s\n",website,inet_ntoa(sockDNS.sin_addr));
                    //Open the file in append mode
                    DNSfile = fopen("DNScache","a");
                    //Add the new found IP to the file
                    fwrite(DNStemp,sizeof(char),strlen(DNStemp),DNSfile);
                    printf("Connected to %s:%hd\n",inet_ntoa(sockDNS.sin_addr),ntohs(sockDNS.sin_port));
                    fclose(DNSfile);
                    break; // if we get here, we must have connected successfully
                }

                if (p == NULL) {
                    // looped off the end of the list with no connection
                    fprintf(stderr, "failed to connect\n");
                    exit(2);
//TODO: Reply Not found error
                }

                freeaddrinfo(servinfo); // all done with this structure
            }
            //Found the website in DNScache, use that to connect
            else{
                printf("Website %s found in cache: %s\n",website,webaddr);
                sockDNS.sin_family = AF_INET;
                sockDNS.sin_port = htons(80);
                sockDNS.sin_addr.s_addr = inet_addr(webaddr);
                if((soc = socket(AF_INET, SOCK_STREAM,0)) == -1) {
                    perror("socket");
                }

                if(connect(soc,(struct sockaddr *)&sockDNS,sizeof(sockDNS)) == -1){
                    perror("connect");
                    close(soc);
                    fprintf(stderr, "failed to connect\n");
                    exit(2);
//TODO: Reply Not found error
                }
            }

            hash =(char *)MD5((unsigned char*)webptr,strlen(webptr),(unsigned char*)md);
            hash = hashedASCII;
            for(int i =0; i<16;i++){
                hash += sprintf(hash,"%0x",md[i]);
            }
            printf("\nThe hash value for %s is %s \n",webptr,hashedASCII);
            SITEfile = fopen(hashedASCII,"r");
            if(SITEfile == NULL){   //Cached file does not exist
                //Send the GET request to the website
                send(soc, copy, rcvd,0);
                //Delete the cached file if it exists
                remove(hashedASCII);
                //Create a new file with the website's URL as name
                //TODO: hash the file name
                SITEfile = fopen(hashedASCII,"w+");
                //Receive the data from the website
                while((rcvd = recv(soc, data_received, sizeof(data_received), 0))){
                    printf("\n\nData bytes %d and message received from website:\n%s\n",rcvd,data_received);
                    //Write the received data to file
                    fwrite(data_received,1,rcvd,SITEfile);
                    //Send the received file without any modifications
                    send(clients[conn], data_received, rcvd,0);
                }
                fclose(SITEfile);
                //Close the socket used to connect to the website
                close(soc);
            }
            else{   //Cached file exists
//TODO: Check for cached file timeout here
                int filesize = get_file_size(hashedASCII);
                int packet, max_packets = filesize/1500;
                printf("\n****************Cached file exists*************\n\n");
                for(packet = 0 ;packet<=max_packets;packet++){
                    //Prepare the packet to be sent
                    bzero(data_to_send,sizeof(data_to_send));
                    //Move the file pointer to the corresponding packet number
                    fseek(SITEfile,packet*1500,SEEK_SET);
                    //Load the file onto the send buffer
                    fread(data_to_send,1500,1,SITEfile);
                    printf("\n\n\n\nData from cache: \n%s\n",data_to_send);
                    //Send the read data to the browser
                    send(clients[conn], data_to_send, 1500,0);
                }
                fclose(SITEfile);
                //Close the socket used to connect to the website
                close(soc);
            }
        }
        else{//Send HTTP/1.1 400 Bad Request
            if((req_ptr = strchr(reqline[0],'\r'))!=NULL)   //If only the method is specified
                strcpy(req_ptr,"\0");
            bzero(data_to_send,sizeof(data_to_send));
            //Prepare the send buffer with the bad request header
            offset = sprintf(sendbuf,"HTTP/1.1 400 Bad Request\n");
            sendbuf+=offset;
            //Append content type
            offset = sprintf(sendbuf,"Content-Type: .html\n");
            sendbuf+=offset;
            bzero(temp,sizeof(&temp));
            //Create the html file content
            strcpy(temp,"<html><body>400 Bad Request Reason: Invalid Method: ");
            strcat(temp,reqline[0]);
            strcat(temp,"</body></html>");
            printf("Temp buffer: %s\n",temp);
            //Append content length
            offset = sprintf(sendbuf,"Content-Length: %d\n\n",(int)strlen(temp));
            sendbuf+=offset;
            //Append the html content
            strcpy(sendbuf,temp);
            //Print the data_to_send for debugging
            printf("data_to_send:\n%s\n",data_to_send);
            //Send the data to the client
            send(clients[conn], data_to_send, strlen(data_to_send),0);
        }
    }

    //Closing SOCKET
shut:printf("Closing socket\n");
    shutdown (clients[conn], SHUT_RDWR);         //All further send and recieve operations are DISABLED
    close(clients[conn]);
    clients[conn]=-1;
}


/* Get the file size in bytes */
int get_file_size(char *filename){
    struct stat buf;
    stat(filename, &buf);
    int size = buf.st_size;
    return size;
}


//Get the pointer to the value for the received item in the conf file
char * getvalue(char item[],char *file){
    if(item == NULL){
        printf("Invalid input\n");
        return NULL;
    }

    char *needle;
    FILE *fp;
    fp = fopen(file,"r");
    if(fp == NULL){
        printf("File %s does not exist for item %s \n",file,item);
        conf_missing = true;
        return NULL;
    }
    else
    {
        conf_missing = false;
        while(fgets(line,sizeof(line),fp)!=NULL){
            if(line[0]=='#')
                continue;
            if((strncmp(line,item,strlen(item)))==0){
                needle = strchr(line,' ');
                needle++;
                strcpy(strchr(needle,'\n'),"\0");
                printf("The value for %s is : %s\n",item,needle);
                fclose(fp);
                return needle;
            }
        }
    }
    fclose(fp);
    printf("Item %s not found\n",item);
    return NULL;
}

int checkInFile(char item[],char *file){
    if(item == NULL){
        printf("Invalid input\n");
        return 0;
    }
    FILE *fp;
    fp = fopen(file,"r");
    if(fp == NULL){
        printf("File %s does not exist for item %s \n",file,item);
        return 0;
    }
    else{
        while(fgets(line,sizeof(line),fp)!=NULL){
            if(line[0]=='#')
                continue;
            if((strncmp(line,item,strlen(item)))==0){
                printf("The item %s is available is %s\n",item,file);
                fclose(fp);
                return 1;
            }
        }
    }
    fclose(fp);
    printf("Item %s not found\n",item);
    return 0;
}
