#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>


//CANDITATENUMBER 248

//Function declarations
void currDir(int sock,int state);
void lsDir();
void cd(int socket);
void cdMenu();
void fInfo(int sock);
void buildLSlist(char out[2000]);
char curDir[2000];
void printMenu(int sock);
void printFile(int sock); 

int main(int argc,char**argv) 
{	
	 if (argc!=2) 
	 {	 
      fprintf(stderr, "ERROR: %s needs two arguments\n", argv[0]);
      exit(-1);
	 }
	
	
	
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	int clientaddrlen;
	int request_sd, client_sock, err;
	char received_data[200];
	
	//Creating the socket
	request_sd = socket(PF_INET,
	 SOCK_STREAM,
	 IPPROTO_TCP);
	 
	//Address structure
	memset(&serveraddr, 0,
	 sizeof(struct sockaddr_in));
	 
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(atoi(argv[1])); 
		
	//Binding, error handling beneath
	err = bind(request_sd,
	 (struct sockaddr*)&serveraddr,
	 sizeof(struct sockaddr_in));
	
	if(err == -1) //If bind returns an error this will give us it verbose.
	{
		perror("Server : Bind");
		exit(-1);
	}
	
	//Queue
	if(listen(request_sd, SOMAXCONN) > 0)
	{
		perror("Serv listen:");
	}
	
	puts("Listening for connections\n");
	
	//This is where the server forks for new clients aka on connections.
	int pid,fork_sock;
	
	for(;;)
	{
		//The connection is accepted / declined and the communication is ready to begin
		clientaddrlen =	 sizeof(struct sockaddr_in);
		
		//fork_sock is the "global" socket for the client.
		//It will be passed around the different functions
		fork_sock = accept(request_sd,
					  (struct sockaddr*)&clientaddr,
					  &clientaddrlen);	
					  
	char ipstr[INET_ADDRSTRLEN];//Set the str to be "ip"-long
	
	//intet_ntop allows for conversion between the bit state and the "verbose" state
	//of the ip address, we get this from the clientaddr struct.
	inet_ntop(AF_INET, &(clientaddr.sin_addr), ipstr, INET_ADDRSTRLEN);
	
	//Prints the clients ip
	printf("IP address %s connected!\n",ipstr); 
		
		//Some fork management
		if ((pid = fork()) == -1)
		{
			close(fork_sock);
			continue;
		}
		else if(pid > 0)
		{
			close(fork_sock);
			continue;
		}
		else if(pid == 0) 
		{			
			int read_size;
			
			//Main loop for the server, here we receive messages and commands from the client.
			while(1)
			{
				if(read(fork_sock ,received_data,200) == 0)
				{
					perror("Read cmd main"); //Error handling
				}
				
				//Switch checks what command has been sent, this only applies for the main
				//menu, functions handle their own messages.
				switch (received_data[0]) 
				{
					case '1':				
						lsDir(fork_sock); //ls
					break;				
					case '2':
						currDir(fork_sock,1); //pwd
							
					break;				
					case '3':
						cd(fork_sock);  //cd
					break;
					case '4':
							fInfo(fork_sock);//stat
					break;
					case '5':
							printFile(fork_sock); //cat
					break;
						case '?':
							printMenu(fork_sock);//simply sends the menu
					break;
					case 'q': //exit case
						printf("%s disconnected!\n",ipstr);
						close(fork_sock); 
						close(request_sd);
						exit(1);
						break;
					break;
					default:
						printf("Error: Invalid command\n");
						
						break;		
				}
		}	}
	}	
        
    return 0;
}
/* 
In this function we find 
all files, folders, and directories etc.
This is done by using the popen() 
function, alternativly one can use 
system ls and > out.txt to pipe 
the result.

*/
void lsDir(int sock) 
{
	//Start by updating the current directory in case
	//this hasn't been done in a while.
	currDir(sock,0);	
	
	//Various buffers
	char tmp[200];
	char buff[2000];
	char test[200][200];
	strcpy(tmp,"ls "); //tmp form the basis for your popen call
	//strcat lets us call "ls /currentPath/" to popen for better results
	strcat(tmp,curDir);
	
	//popen-call
	FILE* file = popen(tmp,"r");
	
	fread(buff,sizeof(char),2000,file); //Read the popen-file
		
	pclose(file);
	
	if(sock == -1)
	{
		perror("lsDir");
        exit(1);
	} else {
		write(sock,&buff,sizeof(buff)); //Check socket, then send to client
	}

}
/*
Returns the current working
directory by using the pwd
*/
void currDir(int sock,int state)
{	
	memset(curDir,0,sizeof(curDir));

	//If the function is called by another function we skip sending
	//the data.
	getcwd(curDir,sizeof(curDir));
	if(state == 1) {	
		if(send(sock,&curDir,sizeof(curDir),0) > 0)
		{
			perror("currDir : sending directory");
		}
	}	
}

void cd(int sock)
{	
	//More buffers, this time with EOF as i experienced some issues with this
	char temp[200];
	temp[199] = '\0';
	char temp2[200];
	temp2[199] = '\0';
	char buff[1000];
	buff[999] = '\0';
	int err;
	
	//Contains the cd menu underneath us.
	//Constructed from snprintf
	char menuString[400];
	
	snprintf(menuString,sizeof(menuString), "%s%s%s%s","Commands for cd:\n ",
								" .. the parent directory\n ",
								" / a new absolute directory\n ",
								" a new directory relative to the current position\n");			
	
	
	send(sock,&menuString,sizeof(menuString),0);
	//Send and recv answer
	recv(sock ,temp2,200,0);
	
	int test = strlen(temp2);
	temp2[test-1] = '\0'; //EOF again
	
	if(err = chdir(temp2) < 0) 
	{
		perror("cd : chdir call");
		exit(0);
	}
		
	//Set new CurDir and send this to the client	
	getcwd(curDir,sizeof(curDir));	
	
	send(sock,&curDir,sizeof(curDir),0);	

}
/*
Gives us a list of
files in the directory and
then we can choose one and
check its filetype. 
This is done by using stat and the 
st_mode inside the stat truck
*/
void fInfo(int sock) 
{
	struct stat fStat; //stat struck, gives us access to much needed info
	char buff[200][200];
	char input[100];
	char word;
	
	
	FILE* file = popen("ls","r"); 
	int counter = 0; //Counter for navigating buff
	fflush(stdin); //fflushing stdin
	while(fgets(buff[counter],sizeof(buff[counter]),file) != NULL) 
	{
	  counter = counter + 1;
	}
   
	pclose(file);
	
	char test[2000];
	char test2[2000];
	strcpy(test2,"Get file info for: \n");
	int i;
	//Once again building a menu for the client, snprintf comes in handy!
	for(i = 0; i < counter-1; i++)
	{
		snprintf(test,sizeof(test), "%s%d%s%s","[",(i+1),"] ",buff[i]);
		strcat(test2,test);
	}
	
	//Send the constructed menu
	if(send(sock,&test2,sizeof(test2),0) > 0)
	{
		perror("fInfo : send file options");
	}
	
	//THe file chosen by the user
	if(read(sock ,input,100) > 0)
	{
		perror("fInfo : read_input");
	}
	
	//Creating nr as a shortcut for the files position.
	int nr = atoi(input) -1;
	
	snprintf(test,sizeof(test),"%s%s",buff[nr]," is a");
	
	//Calling on stat looking for file buff[nr] in the current directory
	if(stat(buff[nr],&fStat) < 0 )
	{
		perror("stat call :");
		//exit(-1);
	}
	
	//Here we start checking what sort of type a given file is
	//THis is stored in the stat struct in the field S_IFMT.
	//By using S_IS REG/DIR/BLK/LNK we can determine the "filetyp"
	
	if((fStat.st_mode & S_IFMT) == S_IFREG) 
	{
		strcat(test," regular file");
	}
	else if((fStat.st_mode & S_IFMT) == S_IFDIR)
	{
		//Directory
		strcat(test," directory");
	}
	else if((fStat.st_mode & S_IFMT) == S_IFBLK)
	{
		//Katalog(??)
		strcat(test," block");
	}
	else if((fStat.st_mode & S_IFMT) == S_IFLNK)
	{
		//link
		strcat(test," link");
	}
	else
	{
		strcat(test," special file");
	}
	//Send the constructed string to the client
	send(sock,&test,sizeof(test),0);	
}

//More or lress self-explanatory
//Sends the main menu tot he user
void printMenu(int sock)
{
	char temp[200];
	snprintf(temp,sizeof(temp),"%s%s%s%s%s%s%s%s","Please press a key:\n",
							"[1] list content of current directory (ls)\n",
							"[2] print name of current directory (pwd)\n",
							"[3] change current directory (cd)\n",
							"[4] get file information\n",
							"[5] display file (cat)\n",
							"[?] this menu\n",
							"[q] quit");
			
	if(send(sock,&temp,sizeof(temp),0) > 0)
	{
		perror("printMenu : send menu");
	}		
			
			
}

/*
This function will list all filed / folders in a directory
and allow the user to choose on fro printing.
The file will be loaded through several buffers and checks to ensure its
printability. Once again we use popen.
*/
void printFile(int sock) 
{
	int counter = 0;
	char input[30];
	char files[100][400]; //Filename "carrier"
	char buffer1[800];
	char buffer2[800];
	
	FILE* file = popen("ls ","r");
	
	while(fgets(files[counter],sizeof(files[counter]),file) != NULL) 
	{
	  counter = counter + 1;
	}
	
	pclose(file);
	
	int i;
	//Another menu
	for(i = 0; i < counter-1; i++) 
	{
		snprintf(buffer1,sizeof(buffer1), "%s%d%s%s%s","[",(i+1),"] ",files[i],"\n");	
		strcat(buffer2,buffer1);
	}
	
	write(sock,buffer2,sizeof(buffer2));
	
	memset(buffer1,0,sizeof(buffer1));
	
	read(sock,input,sizeof(input));
	
	char navnBuff[200];
	navnBuff[199] = '\0';	
	strcpy(navnBuff,files[atoi(input) -1]);	
	
	char miniBuff[200];
	miniBuff[199] = '\0';
	
	currDir(sock,0);
	
	snprintf(miniBuff,sizeof(miniBuff),"%s%s%s%s","cat ",curDir,"/",navnBuff);
	
	printf("%s\n",miniBuff);
	
	file = popen(miniBuff,"r");
	
	char puffBuff[1000];
	
	char sendthis[10000];
	char c_str[1];
	char charr;
	int k;
	//Here we use the function isprint to check if a given char is printable
	//If it is we add it to the output buffer as itself,
	//unprintable chars are presented by .
	
	fflush(stdin);
	while((charr = fgetc(file)) != EOF)
	{
		if(charr == '\n')
		{
			c_str[0] = charr;
			strcat(sendthis,c_str);
			
		}
		else if(isprint(charr) > 0)
		{
			c_str[0] = charr;
			strcat(sendthis,c_str);
		} else {
		
			c_str[0] = '.';
			strcat(sendthis,c_str);	
		}
	}
	
	if(send(sock,&sendthis,sizeof(sendthis),0) > 0)
	{
		perror("printFile : content send");
	}
	
}