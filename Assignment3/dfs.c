#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

int recv_file(int sock,char * filename);
int send_file(int sock, char * filename);

void list(int sock,char * path,int num)
{
	printf("Inside List FILE");
	char tp[3];
	char command[100];
	char filename[100];
	sprintf(command,"ls %s > list%d.txt",path,num);
	printf("System command %s\n\n",command);
	system(command);
	recv(sock, tp, sizeof(tp), 0);
	sprintf(filename,"list%d.txt",num);
	send_file(sock,filename);
}

//This structure is used to recieve command
struct command
{
	char com[10];
	char folder[100];
	char filename[100];
	char username[20];
	char password[20];
};

//This structure is used to communicate while sending and recieving files
struct header
{
	char filename[100];
	int size;
};

//This function isused to obtain second word while parsing conf file
char * getsecondword(char * str)
{
        while( *str !=' ')      //search for space
        {
                if(*str == '\0')
                        return NULL;
                str++;
        }

        str++;

        *(str+strlen(str)-1) = '\0';

        return str;             //return second word
}

//This function searches for item in the ws.conf file and returns coressponding value
char * getitem(char * word)
{
        char line[100];
        char * rv;

        FILE * fp;
        fp = fopen("dfs.conf","r");
	if(fp == NULL)
	{
		printf("Cannot open file\n");
		return '\0';
	}
        while(fgets(line,sizeof(line),fp) != NULL)      //searches the file line by line
        {
                if(line[0] == '#')
                        continue;
                if(strstr(line,word)!=NULL)
                {

                        rv = getsecondword(line);       //find second word corresspond to item searched
                        fclose(fp);
                        return rv;
                }
                bzero(line,sizeof(line));
        }
        fclose(fp);
        return '\0';
}

//This is function is used to verify if username and password match
int verify(char username[100],char password[100])
{
	int number = atoi(getitem("users"));
	printf("Number of users %d",number);
	printf("%s\n",username);
	printf("%s\n",password);

	int i = 1;
	char string[100];
	while(i<=number)
	{
		bzero(string,sizeof(string));
		sprintf(string,"username%d",i);
		if(strcmp(getitem(string),username)==0)
		{
			printf("\t\tusername matched\n\n");
			bzero(string,sizeof(string));
			sprintf(string,"password%d",i);
			if(strcmp(getitem(string),password)==0)
				return 1;
		}
		i++;
	}
	return 0;
}

//is called during put operation. recieves all files from client
void put_file(int sock, char * username)
{
	char filename[100];
	struct header head;
	recv(sock,(void *)&head, sizeof(head), 0);
	sprintf(filename,"./%s/%s",username,head.filename);
	recv_file(sock, filename);
	bzero(head.filename,sizeof(head.filename));
	bzero(filename,sizeof(filename));
	recv(sock,(void *)&head, sizeof(head), 0);
	sprintf(filename,"./%s/%s",username,head.filename);
	recv_file(sock, filename);
}

//Is called during get operation, sends files to client
void get_file(int sock,char * filename)
{
	printf("Inside get_file\n%s\n",filename);
	FILE * fp;
	char name[100];
	char buf[3];
	int sent=0;
	struct header head;
	for(int i=0; i<4;i++)
	{
		bzero(name,sizeof(name));
		sprintf(name,"%s.%d",filename,i+1);
		fp = fopen(name,"r");
		if(fp)
		{
			sent++;
			fclose(fp);
			recv(sock,buf,3,0);
			printf("File hai\n\n");
			send_file(sock,name);
			sleep(1);
		}
	}
	if(sent == 0)
	{
		head.size = 0;
		strcpy(head.filename,"Not Found");
		recv(sock,buf,3,0);
		send(sock, (void*)&head, sizeof(head),0);
		recv(sock,buf,3,0);
		send(sock, (void*)&head,sizeof(head),0);
	}
}
//This function sends big file to client
int send_file(int sock, char *file_name)
{
	printf("Inside send file\n\n");
	int sent_count; /* how many sending chunks, for debugging */
	ssize_t read_bytes, /* bytes read from local file */ sent_bytes, /* bytes sent to connected socket */ sent_file_size;
	char send_buf[1000]; /* max chunk size for sending file */
	char * errmsg_notfound = "File not found\n";
	FILE * f; /* file handle for reading local file*/
	sent_count = 0; sent_file_size = 0;
	 /* attempt to open requested file for reading */
	f = fopen(file_name,"r");
	if(f)
	{
		fseek(f,0,SEEK_END);
		int size = ftell(f);
		rewind(f);
		struct header head;
		sprintf(head.filename,"%s",file_name);
		head.size = size;
		send(sock,(void*)&head, sizeof(head),0);
	}
	if( f < 0) /* can't open requested file */
	{
		printf("Cannot open %s\n\n",file_name);
		if( (sent_bytes = send(sock, "Not found" , 10, 0)) < 0 )
		{
			printf("send error");
			return -1;
		}
	}
	else /* open file successful */
	{
		printf("Sending file: %s\n", file_name);
		while( (read_bytes = fread(send_buf,1,1000,f)) > 0 )
		{
			if( (sent_bytes = send(sock, send_buf, read_bytes, 0)) < read_bytes )
			{
				printf("send error");
				return -1;
			}
			sent_count++;
//			printf("%d\n",sent_count);
			sent_file_size += sent_bytes;
		}
		printf("Bytes Read : %ld\n",read_bytes);
		fclose(f);
	} /* end else */
	printf("Done with this client. Sent %ld bytes in %d send(s)\n\n", sent_file_size, sent_count);
	return sent_count;
}

//this function recieves big file from client
int recv_file(int sock, char* file_name)
{
	char send_str [1000]; /* message to be sent to server*/
	FILE * f; /* file handle for receiving file*/
	ssize_t sent_bytes, rcvd_bytes, rcvd_file_size;
	int recv_count; /* count of recv() calls*/
	char recv_str[1000]; /* buffer to hold received data */
	size_t send_strlen; /* length of transmitted string */
	sprintf(send_str, "%s\n", file_name); /* add CR/LF (new line) */
	send_strlen = strlen(send_str); /* length of message to be transmitted */
	printf("Recieving file\n%s\n",file_name);
	/* attempt to create file to save received data. 0644 = rw-r--r-- */
	if ( (f = fopen(file_name, "w")) < 0 )
	{
		printf("error creating file");
		return -1;
	}
	recv_count = 0;
	/* number of recv() calls required to receive the file */
	rcvd_file_size = 0;
	while ( (rcvd_bytes = recv(sock, recv_str, 1000, 0)) > 0 )
	{
		recv_count++;
		rcvd_file_size += rcvd_bytes;
		if (fwrite(recv_str, rcvd_bytes,1,f) < 0 )
		{
			perror("error writing to file");
			return -1;
		}

		if(rcvd_bytes<1000)
			break;
	}
	fclose(f); /* close file*/
	printf("Client Received: %ld bytes in %d recv(s)\n", rcvd_file_size, recv_count);
	return rcvd_file_size;
}

int main (int argc, char * argv[])
{
	int listenfd, connfd, n;
	socklen_t clilen;
	char buf[5000000];
	struct sockaddr_in cliaddr, servaddr;

	int clients[50];
	int slot = 0;
	for (int i=0; i<50; i++)
		clients[i]=-1;

    printf("Starting server at port no. %s with root directory as /%s/\n",argv[2],argv[1]);
//creation of the socket
	listenfd = socket (AF_INET, SOCK_STREAM, 0);

//preparation of the socket address
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[2]));

	bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	listen (listenfd,1000000);

	struct command c;
	clilen = sizeof(cliaddr);
	while(1)
	{
		clients[slot] = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);

		if(fork())
		{
			connfd = clients[slot];
			bzero(c.username,sizeof(c.username));
			bzero(c.password,sizeof(c.password));
			bzero(c.com,sizeof(c.com));
			bzero(c.filename,sizeof(c.filename));

			recv(connfd, (void *)&c, sizeof(c),0);

			printf("Verification %d",verify(c.username,c.password));

			if(verify(c.username,c.password)==0)
			{
				printf("Username and Password Don't Match\n\n");
				shutdown(connfd,SHUT_RDWR);
                   		close(connfd);
                        	clients[slot] = -1;
			}

			printf("Command: %s\n",c.folder);

			char string[100];

			sprintf(string,"./%s/%s/%s/%s",argv[1],c.username,c.folder,c.filename);
			sprintf(c.filename,"%s",string);
			bzero(string,sizeof(string));
			sprintf(string,"./%s/%s",argv[1],c.username);
			sprintf(c.username,"%s",string);
			printf("%s",c.filename);
			bzero(string,sizeof(string));
			sprintf(string,"%s/%s",c.username,c.folder);
			sprintf(c.folder,"%s",string);
		//	bzero(string,sizeof(string));

			printf("\t\tFOLDER = %s\n\n",string);

			bzero(string,sizeof(string));

			DIR * folder = opendir(c.folder);

			if(ENOENT == errno)
			{
				sprintf(string,"mkdir %s",c.folder);
				system(string);
				bzero(string, sizeof(string));
			}
			closedir(folder);

			DIR * dir = opendir(c.username);

			if(ENOENT == errno)
			{
				sprintf(string,"mkdir %s",c.username);
				system(string);
				bzero(string, sizeof(string));
			}
			closedir(dir);

			if(!strcmp(c.com,"PUT"))
				put_file(connfd,c.folder);
			else if(!strcmp(c.com,"GET"))
				get_file(connfd,c.filename);
			else if(!strcmp(c.com,"LIST"))
				list(connfd, c.folder, atoi(argv[2])%100000);

			shutdown(connfd,SHUT_RDWR);
			close(connfd);
			clients[slot] = -1;
		}
		while (clients[slot]!=-1) slot = (slot+1)%50;
	}
}
