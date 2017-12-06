#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<time.h>
#include<unistd.h>
#include<openssl/md5.h>
#include<openssl/hmac.h>
#include<sys/stat.h>

int send_file(int sock, char * filename);
int splitBigFile(char * file_name);
int mergeBigFile(char * file_name);
int recv_file(int sock, char * filename, int size);
char * getfilename(char * line);
void displaylist();
void list(int * clientSockets, int * connected);
int get_file_size(char *filename);
int crypt_file(char * filename, char * crypted, char * key);
int getorder(char * filename);
void add_to_list(char * filename, int * clientSocket);
void display_list();
void get_file(int * clientsocket, char * filename, int * connected);
void put_file(int * clientsocket, char * filename, int * connected);
void parse_conf_file(void);
char * getvalue(char item[]);


char line[256];

struct command
{
        char com[10];
        char folder[100];
        char filename[100];
        char username[20];
        char password[20];
};

//this structure is used to communicate file header
struct header
{
        char filename[100];
        int size;
};


char * getfilename(char * line)
{
        int length = strlen(line);
        char * str = line+length;
	char * folder = line+length+1;
	while(length > 0)
        {
                if(*str=='.')
                        break;
                str--;
                length--;
        }
        if(length == 0)
	{
//		bzero(folder,sizeof(folder));
//		printf("FOLDER MILA HAI\n\n");
		*(str+strlen(str)-1)=' ';
		sprintf(folder,"/%s FOLDER",str);
		return folder;
        }
	else
        {
                *str = '\0';
                return line;
        }
}
void displaylist()
{
        FILE * fp;
        char line[100];
        char filename[100];
        char old_filename[100];
        int count = 0;
        fp = fopen("list.txt","r");
        if(fgets(line,sizeof(line),fp)!=NULL)
        {
                strcpy(old_filename,getfilename(line));
                count = 1;
        }
        printf("\n\n------------------LIST----------------------\n\n");
	while(fgets(line,sizeof(line),fp)!=NULL)
        {
//		printf("\n\n -----LIST-----\n\n");

//              printf("Scanned from line\n%s\n",line);
                sprintf(filename,"%s",getfilename(line));
//              printf("Filename is :\n%s\n",filename);
		if(filename[0] == '/')
		{
			printf("%s\n",filename);
			continue;
		}
		else if(strcmp(filename,old_filename)==0)
                {
                        count++;
                }
                else
                {
                        if(count == 4)
                                printf("%s\n",old_filename);
                        else
			{
                                printf("%s INCOMPLETE\n",old_filename);
                        }
			count = 1;
                        strcpy(old_filename,filename);
                }
        }
        if(count == 4)
                printf("%s\n",old_filename);
        else
                printf("%s INCOMPLETE\n",old_filename);
}

void list(int * clientSockets, int * connected)
{
    system("rm -rf list?.txt");
	printf("Inside List file\n\n");
	struct header head;
	int i = 0;
	char filename[100];
	for(i=0;i<4;i++)
	{
		bzero(filename,sizeof(filename));
		sprintf(filename,"list%d.txt",i);
		if(*(connected+i) == 0)
		{
			sleep(1);
			send(*(clientSockets+i),"abc",3,0);
			printf("sent abc\n");
			recv(*(clientSockets+i),(void *)&head,sizeof(head),0);
			printf("recieved file header\n");
			recv_file(*(clientSockets+i),filename,head.size);
		}
	}
	system("sort -u list* > list.txt");
	displaylist();
}

//This function returns the filezise of any file
int get_file_size(char *filename)
{
    struct stat buf;
    stat(filename, &buf);
    int size = buf.st_size;
    return size;
}

//This file generates a encrypted file
int crypt_file(char * filename, char * crypted, char * key)
{
    FILE *fp, *fd;
    int file_size = get_file_size(filename);
    char* crypted_temp = "crypted_temp";
    //Open source file for reading
    fp = fopen(filename,"r");
    if(fp == NULL ){
        printf("File does not exist\n");
        return 0;
    }
    //Create a temporary file for storing the XORed data
    fd = fopen(crypted_temp,"w+");
    if(fp == NULL ){
        printf("File does not exist\n");
        return 0;
    }

    //Current data byte
    char data_byte;
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

//This function returns md5 value for order in which the file is uploaded
int getorder(char * filename)
{
	printf("getting order");
	char buffer[512];
	FILE * fp;
	MD5_CTX c;
	int read;
	unsigned char out[MD5_DIGEST_LENGTH];
	fp = fopen(filename,"r");
	read = fread(buffer,sizeof(buffer),1,fp);
	MD5_Init(&c);
	int i;
	while(read!=0)
	{
		MD5_Update(&c,buffer,read);
		read = fread(buffer,sizeof(buffer),1,fp);
	}
	MD5_Final(out,&c);

	return out[MD5_DIGEST_LENGTH]%4;
}

//This function adds to list file for list functionality
void add_to_list(char * filename, int * clientSocket)
{
	printf("ADDED TO LIST\n\n\n");

	FILE * file;
	char string[100];
	char buffer[1000];
	char write_buffer[1000];
	bzero(string,sizeof(string));
	bzero(buffer,sizeof(buffer));
	bzero(write_buffer,sizeof(write_buffer));
	file = fopen("list.txt","a");
	if((*clientSocket + *(clientSocket+2) ==0) || ( *(clientSocket+1) + *(clientSocket+3))==0)
		sprintf(string,"%s COMPLETE\n",filename);
	else
		sprintf(string,"%s INCOMPLETE\n",filename);
	printf("Writing %s\n",string);
	fwrite(string,strlen(string),1,file);
	fclose(file);
}

//This function is used to display the list
void display_list()
{
	FILE * fp;
	char buffer[10000];
//	system("sort -u list.txt > list.txt");
	fp = fopen("list.txt","r");
	fread(buffer,sizeof(buffer),1,fp);
	printf("LIST: %s",buffer);
	fclose(fp);
}

//this is function is used to obtain file from server for Get command
void get_file(int * clientsocket, char * filename, int * connected)
{
	printf("Inside GEt File\n\n");
	char command[100];
	struct header head;
	char file [100];
	int count=0;
	for(int i = 0;i<4;i++)
	{
		if(*(connected+i)!=-1)		//receive files only from connected servers
		{
			send(*(clientsocket+i),"abc",3,0);
			recv(*(clientsocket+i),(void *)&head,sizeof(head),0);
			printf("Filename %s SIZE %d\n\n",head.filename,head.size);
			if(head.size!=0)
			{
				count ++;
				sprintf(file,"%s.%c",filename,head.filename[strlen(head.filename)-1]);
				recv_file(*(clientsocket+i),file,head.size);
			}
			bzero(file,sizeof(file));
			bzero(head.filename,sizeof(head.filename));
			head.size = 0;
			send(*(clientsocket+i),"abc",3,0);
			recv(*(clientsocket+i),(void *)&head,sizeof(head),0);
			printf("Filename %s SIZE %d\n\n",head.filename,head.size);
			if(head.size!=0)
			{
				count++;
				sprintf(file,"%s.%c",filename,head.filename[strlen(head.filename)-1]);
				recv_file(*(clientsocket+i),file,head.size);
			}
		}
	}
	if(count>0)
		mergeBigFile(filename);
	bzero(command,sizeof(command));
        sprintf(command,"rm %s.?",filename);
	system(command);
}

//This function is called to put files in desired order to server
void put_file(int * clientsocket, char * filename, int * connected)
{
	char name[100];
	splitBigFile(filename);
	int order = getorder(filename);
	if(order == 0)
	{
		int part[5] = {1,2,3,4,1};
		for(int i = 0; i<4; i++)
		{
			if(*(connected+i)!=-1)
			{
				bzero(name,sizeof(name));
				sprintf(name,"%s.%d",filename,part[i]);
				send_file(*(clientsocket+i),name);
				sleep(1);
				bzero(name,sizeof(name));
				sprintf(name,"%s.%d",filename,part[i+1]);
				send_file(*(clientsocket+i),name);
			}
		}
	}
	else if(order == 1)
	{
		int part[5] = {4,1,2,3,4};
                for(int i = 0; i<4; i++)
                {
			if(*(connected+i)!=-1)
			{
                        	bzero(name,sizeof(name));
                        	sprintf(name,"%s.%d",filename,part[i]);
                        	send_file(*(clientsocket+i),name);
                        	sleep(1);
				bzero(name,sizeof(name));
                        	sprintf(name,"%s.%d",filename,part[i+1]);
                        	send_file(*(clientsocket+i),name);
                        }
                }
	}
	else if(order == 2)
	{
		int part[5] = {2,3,4,1,2};
                for(int i = 0; i<4; i++)
                {
			if(*(connected+i)!=-1)
                        {
				bzero(name,sizeof(name));
                	        sprintf(name,"%s.%d",filename,part[i]);
                	        send_file(*(clientsocket+i),name);
                	        sleep(1);
				bzero(name,sizeof(name));
                        	sprintf(name,"%s.%d",filename,part[i+1]);
                        	send_file(*(clientsocket+i),name);
                        }
         	}
	}
	else
	{
		int part[5] = {3,4,1,2,3};
                for(int i = 0; i<4; i++)
                {
			if(*(connected+i)!=-1)
			{
                        	bzero(name,sizeof(name));
                        	sprintf(name,"%s.%d",filename,part[i]);
                        	send_file(*(clientsocket+i),name);
                        	sleep(1);
				bzero(name,sizeof(name));
                        	sprintf(name,"%s.%d",filename,part[i+1]);
                        	send_file(*(clientsocket+i),name);
                        }
                }
	}
	char command[100];
	sprintf(command,"rm %s.*",filename);
	system(command);
}
//this function is called to send large files via TCP
int send_file(int sock, char * file_name)
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
	fseek(f,0,SEEK_END);
	int size = ftell(f);
	rewind(f);
	char arr[100];
	printf("\t\tSIZE %d\n\n",size);
	sprintf(arr,"%d",size);
	struct header head;
	sprintf(head.filename,"%s",file_name);
	head.size = size;
	send(sock,(void *)&head,sizeof(head),0);
	if( f < 0) /* can't open requested file */
	{
		printf("Cannot open %s\n\n",file_name);
		if( (sent_bytes = send(sock, errmsg_notfound , strlen(errmsg_notfound), 0)) < 0 )
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
			sent_file_size += sent_bytes;
		}
		printf("Bytes Read : %ld\n",read_bytes);
		fclose(f);
	} /* end else */
	printf("Done with this client. Sent %ld bytes in %d send(s)\n\n", sent_file_size, sent_count);
	return sent_count;
}

//this function recieves large file from socket via TCP
int recv_file(int sock, char* file_name, int size)
{
	printf("\t\tSIZE = %d\n\n\n",size);

	char send_str [1000]; /* message to be sent to server*/
	FILE * f; /* file handle for receiving file*/
	ssize_t sent_bytes, rcvd_bytes, rcvd_file_size;
	int recv_count; /* count of recv() calls*/
	char recv_str[1000]; /* buffer to hold received data */
	size_t send_strlen; /* length of transmitted string */
	sprintf(send_str, "%s\n", file_name); /* add CR/LF (new line) */
	send_strlen = strlen(send_str); /* length of message to be transmitted */
	printf("Recieving file\n\n");
	/* attempt to create file to save received data. 0644 = rw-r--r-- */
	if ( (f = fopen(file_name, "w")) < 0 )
	{
		perror("error creating file");
		return -1;
	}
	recv_count = 0;
	/* number of recv() calls required to receive the file */
	rcvd_file_size = 0;
	/* size of received file */ /* continue receiving until ? (data or close) */
	while ( (rcvd_bytes = recv(sock, recv_str, 1000, 0)) > 0 && rcvd_file_size<size)
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

//This function is used to obtain second word from conf file
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
        fp = fopen("dfc.conf","r");
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

//This function is called to split large file into 4 parts
int splitBigFile(char * filename)
{
        FILE * fp;
        long size,parts = 4;

	long maxsize = 5000000;

        char buffer[maxsize];
        char partname[100];

        fp = fopen(filename,"r");
        fseek(fp,0,SEEK_END);
        size = ftell(fp);
        rewind(fp);

        FILE * temp;
        int i = parts;
        int j = 0;
        long read;
        while(i>0)
        {
//		printf("Part %d Worked\n\n",i);
                bzero(buffer, sizeof(buffer));
                bzero(partname,sizeof(bzero));
                sprintf(partname,"%s.%d",filename,i--);
                temp = fopen(partname,"w");
                if(i == 0)
                {
                        read = fread(buffer,1,sizeof(buffer),fp);
                        fwrite(buffer,1,read,temp);
                }
                else
                {
                        read = fread(buffer,1,size/parts,fp);
                        fwrite(buffer,1,read,temp);
                }
                fclose(temp);
        }
        fclose(fp);

	printf("\n\nFILE SPLIT\n\n");
}

//This function is used to merge 4 parts
int mergeBigFile(char * filename)
{
	FILE * fp;
	FILE * temp;
	long long maxsize = 5000000;
	char partname[100];
	char command[100];
	char buffer[maxsize];
	int parts = 4;
	long long read = 0;
	fp = fopen(filename,"w");
	while(parts>0)
	{
//		printf("Part %d worked\n\n",parts);
		bzero(buffer,sizeof(buffer));
		bzero(partname,sizeof(partname));
		sprintf(partname,"%s.%d",filename,parts);
		temp = fopen(partname,"r");
		if(temp == NULL)
		{
			printf("\n\nFILE INCOMPLETE\n\n");
			sprintf(command,"rm %s*",filename);
			system(command);
			break;
		}
		read = fread(buffer,1,sizeof(buffer),temp);
		printf("Read = %lld",read);
		if(parts == 1)
			fwrite(buffer,1,read,fp);
		else
			fwrite(buffer,1,read,fp);
		parts--;
		fclose(temp);
	}
	fclose(fp);
}

int main()
{
	int clientSocket[4];
	char buffer[1024];
	struct sockaddr_in serverAddr[4];
	int connected[4];
	char * string;
	int bytes;

    parse_conf_file();
    printf("\nServers should be active on ports %s ", getitem("DFS1PORT"));
        printf("%s ",getitem("DFS2PORT"));
        printf("%s ",getitem("DFS3PORT"));
        printf("%s\n",getitem("DFS4PORT"));
    printf("Client started for user: %s\n",getitem("Username"));

	while(1)
	{
		struct command c;

		bzero(c.username,sizeof(c.username));
		bzero(c.password,sizeof(c.password));
		bzero(c.filename,sizeof(c.filename));
		bzero(c.folder,sizeof(c.folder));
		bzero(c.com,sizeof(c.com));

        printf("\n*********************************************************\n");
        printf("Commands accepted\n#  LIST\n#  GET\n#  PUT\n#  EXIT\n");
        printf("*********************************************************\n");
        printf("\nWaiting for command:\n");
        scanf("%s",c.com);

		if(strcmp(c.com,"LIST")==0)
		{
			sprintf(c.username,"%s",getitem("Username"));
        	sprintf(c.password,"%s",getitem("Password"));

			for(int i = 0; i<4; i++)
	        {
                clientSocket[i] = socket(PF_INET, SOCK_STREAM, 0);
                serverAddr[i].sin_family = AF_INET;
                bzero(buffer,sizeof(buffer));
                sprintf(buffer,"DFS%dPORT",i+1);
                string = getitem(buffer);
                sprintf(buffer,"%s",string);
                serverAddr[i].sin_port = htons(atoi(string));
                bzero(buffer,sizeof(buffer));
                sprintf(buffer,"DFS%d",i+1);
                string = getitem(buffer);
                sprintf(buffer,"%s",string);
                serverAddr[i].sin_addr.s_addr = inet_addr(buffer);
                connected[i] = connect(clientSocket[i], (struct sockaddr *) &serverAddr[i], sizeof(serverAddr[i]));
                printf("Server %d Online : %d\n",i,connected[i]);
            }

			for(int i=0; i<4; i++)
            {
                if(connected[i]!=-1)
                    bytes = send(clientSocket[i], (void*)&c, sizeof(c), 0);
                printf("\n%d\n\n",bytes);
            }

			list(clientSocket,connected);

			for(int i =0;i<4;i++)
        	{
            	close(clientSocket[i]);
        	}

			continue;
		}
		else if(strcmp(c.com,"EXIT") == 0)
        {
            for(int i =0;i<4;i++)
            {
                close(clientSocket[i]);
            }
            exit(0);
        }
        else if(strcmp(c.com,"GET")!=0 && strcmp(c.com,"PUT")!=0)
        {
            printf("Invalid Command\n\n");
        	continue;
		}


		printf("Enter Folder\n");
		scanf("%s",c.folder);

		printf("Enter Filename\n");
		scanf("%s",c.filename);

		sprintf(c.username,"%s",getitem("Username"));
		sprintf(c.password,"%s",getitem("Password"));

		//connect to servers
		for(int i = 0; i<4; i++)
        {
            clientSocket[i] = socket(PF_INET, SOCK_STREAM, 0);
            serverAddr[i].sin_family = AF_INET;
   	        bzero(buffer,sizeof(buffer));
            sprintf(buffer,"DFS%dPORT",i+1);
            string = getitem(buffer);
            sprintf(buffer,"%s",string);
            serverAddr[i].sin_port = htons(atoi(string));
            bzero(buffer,sizeof(buffer));
            sprintf(buffer,"DFS%d",i+1);
            string = getitem(buffer);
            sprintf(buffer,"%s",string);
            serverAddr[i].sin_addr.s_addr = inet_addr(buffer);
            connected[i] = connect(clientSocket[i], (struct sockaddr *) &serverAddr[i], sizeof(serverAddr[i]));
            printf("Server %d Online : %d\n",i,connected[i]);

        }

		FILE * fp;

		if(strcmp(c.com,"PUT") == 0)
		{
			printf("Command : %s\n\n",c.com);
			fp = fopen(c.filename,"r");
			if(fp)
			{
				fclose(fp);
				for(int i=0; i<4; i++)
        		{
            		if(connected[i]!=-1)
                		bytes = send(clientSocket[i], (void*)&c, sizeof(c), 0);
            		printf("\n%d\n\n",bytes);
        		}
				put_file(clientSocket,c.filename,connected);
				add_to_list(c.filename,connected);
			}
		}
		else if(strcmp(c.com,"GET") == 0 && ((connected[0]+connected[2]==0) || (connected[1]+connected[3]==0)))
		{
			printf("Command : %s\n\n",c.com);
			for(int i=0; i<4; i++)
	        	{
	                	if(connected[i]!=-1)
	                	        bytes = send(clientSocket[i], (void*)&c, sizeof(c), 0);
	                	printf("\n%d\n\n",bytes);
	        	}
			get_file(clientSocket,c.filename,connected);
		}
		else if(strcmp(c.com,"EXIT") == 0)
		{
			for(int i =0;i<4;i++)
	                {
        	                close(clientSocket[i]);
        	        }
			return 0;
		}

		else
		{
			printf("Invalid Command or File Incomplete\n\n");
		}

		for(int i =0;i<4;i++)
		{
			close(clientSocket[i]);
		}

		printf("\n*****************************************\n");

	}
	return 0;
}


void parse_conf_file(){
    printf("client trying to connect to 127.0.0.1\n");
    printf("The value for Server DFS1 is : DFS1 %s:",getitem("DFS1"));
    printf("%s\n",getitem("DFS1PORT"));
    printf("The value for Server DFS2 is : DFS2 %s:",getitem("DFS2"));
    printf("%s\n",getitem("DFS2PORT"));
    printf("The value for Server DFS3 is : DFS3 %s:",getitem("DFS3"));
    printf("%s\n",getitem("DFS3PORT"));
    printf("The value for Server DFS4 is : DFS4 %s:",getitem("DFS4"));
    printf("%s\n",getitem("DFS4PORT"));
    printf("The value for Username is : %s\n",getitem("Username"));
    printf("The value for Username is : %s\n",getitem("Password"));
}



//Get the pointer to the value for the received item in the conf file
char * getvalue(char item[]){
    if(item == NULL){
        printf("Invalid input\n");
        return NULL;
    }

    char *needle;
    FILE *fp;
    fp = fopen("dfc.conf","r");
    if(fp == NULL){
        printf("File does not exist\n");
        return NULL;
    }
    else
    {
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
    }
    fclose(fp);
    printf("Item %s not found\n",item);
    return NULL;
}
