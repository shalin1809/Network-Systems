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

#define CONNMAX 1000
#define BYTES 1024

void startServer(char *port);
void respond(int slot);
char * getvalue(char item[]);
int get_file_size(char *filename);
char *ROOT;
char line[256];
int listenfd, clients[CONNMAX];
bool conf_missing=false;


int main(int argc, char * argv[]){

    struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char c;
    char *value;
    int slot=0;

    char PORT[6];
    char root[1000];

    //Get port number from conf file
    value = getvalue("Listen");
    if(value==NULL){
        conf_missing=true;
        strcpy(PORT,"8000");
        printf("Conf file missing, setting default port to 8000\n");
    }
    else
        strcpy(PORT,value);

    //Get Document Root from the conf file
    value = getvalue("DocumentRoot");
    if(value==NULL){
        conf_missing=true;
        strcpy(root,getenv("PWD"));
        printf("Conf file missing, setting default root to %s\n",root);
    }
    else
        strcpy(root,value);
    ROOT = root;

    printf("Starting server at port no. %s with root directory as %s\n",PORT,ROOT);
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
void startServer(char *port)
{
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


//client connection
void respond(int n)
{


    struct HTTP_packet_t{
        char* method;
        char* URI;
        char* version;
        char* content_type;
        char* content_length;
    }HTTP_packet;

    char* method_get = "GET";
    char* method_post = "POST";
    char* version_1_0 = "HTTP/1.0";
    char* version_1_1 = "HTTP/1.1";
    char* blankline = "\n\r";
    char *value;

	char mesg[99999], *reqline[3], data_to_send[BYTES], path[99999],copy[99999];
	int rcvd, fd, bytes_read;
    char* sendbuf = data_to_send;
    int offset,size;
    char temp[512];
    char * req_ptr;
    char * POST_DATA;

    //Reset the message buffer
	memset( (void*)mesg, (int)'\0', 99999 );

    //receive the tcp data
	rcvd=recv(clients[n], mesg, 99999, 0);
    strcpy(copy,mesg);

    value = getvalue("Listen");
    //Send internal server error if config file is missing
    if(conf_missing==true){
        send(clients[n],"HTTP/1.1 500 Internal Server Error",34,0);
        goto shut;
    }


	if (rcvd<0)    // receive error
		fprintf(stderr,("recv() error\n"));
	else if (rcvd==0)    // receive socket closed
		fprintf(stderr,"Client disconnected upexpectedly.\n");
	else    // message received
	{
        //print the received message
		printf("received message: %s\n", mesg);
        //Parse the received header
		reqline[0] = strtok (mesg, " \t");
        //Check if it is a GET request
		if ( strncmp(reqline[0], "GET\0", 4)==0 )
		{
			reqline[1] = strtok (NULL, " \t");
			reqline[2] = strtok (NULL, " \t\n");

            //Check the HTTP version
			if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 )
			{
                //Reply bad request if version is not 1.0 or 1.1
                if((req_ptr = strchr(reqline[2],'\r'))!=NULL)
                    strcpy(req_ptr,"\0");
                bzero(data_to_send,sizeof(data_to_send));
                HTTP_packet.version = version_1_0;
                //Set 1st line to say bad request
                offset = sprintf(sendbuf,"%s 400 Bad Request\n",HTTP_packet.version);
                sendbuf+=offset;
                value = getvalue(".html");
                //Append content type
                offset = sprintf(sendbuf,"Content-Type: %s\n",value);
                sendbuf+=offset;
                bzero(temp,sizeof(&temp));
                //Create the html file content
                strcpy(temp,"<html><body>400 Bad Request Reason: Invalid HTTP version: ");
                strcat(temp,reqline[2]);
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
                send(clients[n], data_to_send, strlen(data_to_send),0);
			}
			else
			{
                //Requested file does not have proper path specified
                if(strncmp(reqline[1],"/",1)!=0){
                    if((req_ptr = strchr(reqline[1],'\r'))!=NULL)
                        strcpy(req_ptr,"\0");
                    bzero(data_to_send,sizeof(data_to_send));
                    HTTP_packet.version = version_1_0;
                    //Set 1st line to say bad request
                    offset = sprintf(sendbuf,"%s 400 Bad Request\n",HTTP_packet.version);
                    sendbuf+=offset;
                    value = getvalue(".html");
                    //Append content type
                    offset = sprintf(sendbuf,"Content-Type: %s\n",value);
                    sendbuf+=offset;
                    bzero(temp,sizeof(&temp));
                    //Create the html file content
                    strcpy(temp,"<html><body>400 Bad Request Reason: Invalid URL: ");
                    strcat(temp,reqline[1]);
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
                    send(clients[n], data_to_send, strlen(data_to_send),0);
                }

                //Requested URL starts with a /, thus we consider it to be a valid URL
                else{
                    if ( strncmp(reqline[1], "/\0", 2)==0 ){
                        value = getvalue("DirectoryIndex");
                        reqline[1] = value;        //Because if no file is specified, index.html will be opened by default (like it happens in APACHE...
                    }

                    strcpy(path, ROOT);
                    strcpy(&path[strlen(ROOT)], reqline[1]);
                    printf("file: %s\n", path);
                    //File found
                    if ( (fd=open(path, O_RDONLY))!=-1 )
                    {
                        if((req_ptr = strchr(reqline[2],'\r'))!=NULL)
                            strcpy(req_ptr,"\0");
                        bzero(data_to_send,sizeof(data_to_send));
                        HTTP_packet.version = reqline[2];
                        //Set 1st line to say bad request
                        offset = sprintf(sendbuf,"%s 200 OK\n",HTTP_packet.version);
                        sendbuf+=offset;
                        value = getvalue(strrchr(path,'.'));
                        //Append content type
                        offset = sprintf(sendbuf,"Content-Type: %s\n",value);
                        sendbuf+=offset;
                        offset = sprintf(sendbuf,"Content-Length: %d\n\n",get_file_size(path));
                        printf("\ndata_to_send:\n%s\n", data_to_send);
                        send(clients[n], data_to_send, strlen(data_to_send), 0);
                        while ( (bytes_read=read(fd, data_to_send, BYTES))>0 )
                        write (clients[n], data_to_send, bytes_read);
                    }
                    //File not found
                    else{
                        if((req_ptr = strchr(reqline[1],'\r'))!=NULL)
                            strcpy(req_ptr,"\0");
                        bzero(data_to_send,sizeof(data_to_send));
                        HTTP_packet.version = version_1_0;
                        //Set 1st line to say bad request
                        offset = sprintf(sendbuf,"%s 404 Not Found\n",HTTP_packet.version);
                        sendbuf+=offset;
                        value = getvalue(".html");
                        //Append content type
                        offset = sprintf(sendbuf,"Content-Type: %s\n",value);
                        sendbuf+=offset;
                        bzero(temp,sizeof(&temp));
                        //Create the html file content
                        strcpy(temp,"<html><body>404 Not Found Reason URL does not exist: ");
                        strcat(temp,reqline[1]);
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
                        send(clients[n], data_to_send, strlen(data_to_send),0);
                    }
                }
			}
		}

        else if(strncmp(reqline[0],"POST\0",5)==0){
            reqline[1] = strtok (NULL, " \t");
			reqline[2] = strtok (NULL, " \t");
            if((req_ptr = strpbrk(reqline[2],"\r\n"))!=NULL)
                strcpy(req_ptr,"\0");
            //Check the HTTP version
            if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 )
			{
                //Reply bad request if version is not 1.0 or 1.1
                if((req_ptr = strchr(reqline[2],'\r'))!=NULL)
                    strcpy(req_ptr,"\0");
                bzero(data_to_send,sizeof(data_to_send));
                HTTP_packet.version = version_1_0;
                //Set 1st line to say bad request
                offset = sprintf(sendbuf,"%s 400 Bad Request\n",HTTP_packet.version);
                sendbuf+=offset;
                value = getvalue(".html");
                //Append content type
                offset = sprintf(sendbuf,"Content-Type: %s\n",value);
                sendbuf+=offset;
                bzero(temp,sizeof(&temp));
                //Create the html file content
                strcpy(temp,"<html><body>400 Bad Request Reason: Invalid HTTP version: ");
                strcat(temp,reqline[2]);
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
                send(clients[n], data_to_send, strlen(data_to_send),0);
			}
            else
			{
                *(reqline[2]+8) = '\0';
                //Requested file does not have proper path specified
                if(strncmp(reqline[1],"/",1)!=0){
                    if((req_ptr = strchr(reqline[1],'\r'))!=NULL)
                        strcpy(req_ptr,"\0");
                    bzero(data_to_send,sizeof(data_to_send));
                    HTTP_packet.version = version_1_0;
                    //Set 1st line to say bad request
                    offset = sprintf(sendbuf,"%s 400 Bad Request\n",HTTP_packet.version);
                    sendbuf+=offset;
                    value = getvalue(".html");
                    //Append content type
                    offset = sprintf(sendbuf,"Content-Type: %s\n",value);
                    sendbuf+=offset;
                    bzero(temp,sizeof(&temp));
                    //Create the html file content
                    strcpy(temp,"<html><body>400 Bad Request Reason: Invalid URL: ");
                    strcat(temp,reqline[1]);
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
                    send(clients[n], data_to_send, strlen(data_to_send),0);
                }

                //Requested URL starts with a /, thus we consider it to be a valid URL
                else{
                    //If no file is specified, DirectoryIndex in the conf file will be opened by default
                    if ( strncmp(reqline[1], "/\0", 2)==0 ){
                        value = getvalue("DirectoryIndex");
                        reqline[1] = value;
                    }

                    strcpy(path, ROOT);
                    strcpy(&path[strlen(ROOT)], reqline[1]);
                    printf("file: %s\n", path);
                    //File found
                    if ( (fd=open(path, O_RDONLY))!=-1 )
                    {
                        if((req_ptr = strchr(reqline[2],'\r'))!=NULL)
                            strcpy(req_ptr,"\0");
                        bzero(data_to_send,sizeof(data_to_send));
                        HTTP_packet.version = reqline[2];
                        //Offset calculated by removing the first line and adding 3 NULL
                        offset=strlen(reqline[0])+strlen(reqline[1])+strlen(reqline[2]);
                        //Search for the blank line
                        POST_DATA=copy;
                        while((POST_DATA = strchr(++POST_DATA,'\n'))!=NULL){
                            //printf("\nSearching: %s",POST_DATA);
                            if((strchr(POST_DATA+1,'\n') - POST_DATA)==1)
                                break;
                        };
                        if(POST_DATA == NULL)
                            POST_DATA = "No post data";
                        //Move the pointer ahead of the blank line to get the post data
                        else
                            POST_DATA+=2;
                        printf("postdata: %s\n",POST_DATA );
                        //Set first line to reply OK
                        offset = sprintf(sendbuf,"%s 200 OK\n",HTTP_packet.version);
                        sendbuf+=offset;
                        value = getvalue(strrchr(path,'.'));
                        //Append content type
                        offset = sprintf(sendbuf,"Content-Type: %s\n",value);
                        sendbuf+=offset;
                        //Add content length
                        sprintf(temp,"<html><body><pre><h1>%s</h1></pre>",POST_DATA);
                        offset = sprintf(sendbuf,"Content-Length: %d\n\n",(get_file_size(path)+(int)strlen(temp)));
                        sendbuf+=offset;
                        //Append the post data after a blank line
                        //strcat(sendbuf,temp);
                        printf("\ndata_to_send:\n%s\n", data_to_send);
                        send(clients[n], data_to_send, strlen(data_to_send), 0);
                        write (clients[n], temp, strlen(temp));
                        while ( (bytes_read=read(fd, data_to_send, BYTES))>0 )
                        write (clients[n], data_to_send, bytes_read);
                    }
                    //File not found
                    else{
                        if((req_ptr = strchr(reqline[1],'\r'))!=NULL)
                            strcpy(req_ptr,"\0");
                        bzero(data_to_send,sizeof(data_to_send));
                        HTTP_packet.version = version_1_0;
                        //Set 1st line to say bad request
                        offset = sprintf(sendbuf,"%s 404 Not Found\n",HTTP_packet.version);
                        sendbuf+=offset;
                        value = getvalue(".html");
                        //Append content type
                        offset = sprintf(sendbuf,"Content-Type: %s\n",value);
                        sendbuf+=offset;
                        bzero(temp,sizeof(&temp));
                        //Create the html file content
                        strcpy(temp,"<html><body>404 Not Found Reason URL does not exist: ");
                        strcat(temp,reqline[1]);
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
                        send(clients[n], data_to_send, strlen(data_to_send),0);
                    }   // write(clients[n], "HTTP/1.0 404 Not Found\n", 23); //FILE NOT FOUND
                }
			}
        }

        //Not implemented methods
    	else if ((strncmp(reqline[0],"HEAD\0",5)==0)|| \
                (strncmp(reqline[0],"DELETE\0",7)==0)|| \
                (strncmp(reqline[0],"OPTIONS\0",8)==0)){
            //Reply not implemented for known methods not implemented
            bzero(data_to_send,sizeof(data_to_send));
            HTTP_packet.version = version_1_0;
            //Set 1st line to say bad request
            offset = sprintf(sendbuf,"%s 501 Not Implemented\n",HTTP_packet.version);
            sendbuf+=offset;
            value = getvalue(".html");
            //Append content type
            offset = sprintf(sendbuf,"Content-Type: %s\n",value);
            sendbuf+=offset;
            bzero(temp,sizeof(&temp));
            //Create the html file content
            strcpy(temp,"<html><body>501 Not Implemented Method: ");
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
            send(clients[n], data_to_send, strlen(data_to_send),0);
        }

        //Invalid Method
        else{

            if((req_ptr = strchr(reqline[0],'\r'))!=NULL)
                strcpy(req_ptr,"\0");
            bzero(data_to_send,sizeof(data_to_send));
            HTTP_packet.version = version_1_0;
            //Set 1st line to say bad request
            offset = sprintf(sendbuf,"%s 400 Bad Request\n",HTTP_packet.version);
            sendbuf+=offset;
            value = getvalue(".html");
            //Append content type
            offset = sprintf(sendbuf,"Content-Type: %s\n",value);
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
            send(clients[n], data_to_send, strlen(data_to_send),0);
        }
	}

	//Closing SOCKET
shut:shutdown (clients[n], SHUT_RDWR);         //All further send and recieve operations are DISABLED
	close(clients[n]);
	clients[n]=-1;
}



char * getvalue(char item[]){
    if(item == NULL){
        printf("Invalid input\n");
        return NULL;
    }

    char *needle;
    FILE *fp;
    fp = fopen("ws.conf","r");
    if(fp == NULL){
        printf("File does not exist\n");
        conf_missing = true;
    }
    else
        conf_missing = false;
    while(fgets(line,sizeof(line),fp)!=NULL){
        if(line[0]=='#')
            continue;
        if((strncmp(line,item,strlen(item)))==0){
            needle = strchr(line,' ');
            needle++;
            strcpy(strchr(needle,'\n'),"\0");
            if(item == "DocumentRoot"){
                needle++;
                strcpy(strchr(needle,'"'),"\0");
            }
            printf("The value for %s is : %s\n",item,needle);
            fclose(fp);
            return needle;
        }
    }
    fclose(fp);
    printf("Item %s not found\n",item);
    return NULL;
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
