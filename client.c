#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>
//CANDITATENUMBER 248

void printMenu();

int main(int argc, char *argv[]) //Client
{
	//Checks if the client has the info needed
	 if (argc != 3) 
	 {	 
      fprintf(stderr, "ERROR: %s needs three arguments\n", argv[0]);
      exit(-1);
	 }
	 
	int sockfd, port, n,read_size;
	char hostname[100];	
   
	char str[200]; //used for user input
    char buffer[2000];
	
	
	
	//Decelerations
	struct sockaddr_in serveraddr;
	char buf[13];
	
	//Create socket
	sockfd = socket(PF_INET,
				SOCK_STREAM,
				IPPROTO_TCP);
				
	//Adress
	memset(&serveraddr, 0,
			sizeof(struct sockaddr_in));
			
	//Address family
	serveraddr.sin_family = AF_INET; 
	
	//iP address
	inet_pton(AF_INET, argv[1],
				&serveraddr.sin_addr);
				
	//port
	serveraddr.sin_port = htons(atoi(argv[2]));
	

	//Connecting
	if(connect(sockfd,(struct sockaddr*)&serveraddr,sizeof(struct sockaddr_in)) > 0)
	{
		perror("client: connect");
		exit(1);
	}
	
	//Prints the first menu, after this all menues are sent over the socket.
	printMenu(); 
	while(1)
	{
		memset(buffer,0,sizeof(buffer));
		
		//Get user choice		
		printf("\ncmd> ");
		
		fflush(stdin);
		//Fgets on stdin gives us a good way of getting user input.
		fgets(buffer,sizeof(buffer),stdin);		
		
		//Exit handler
		if(buffer[0] == 'q')
		{
			send(sockfd,&buffer, strlen(buffer),0);	
			close(sockfd);
			exit(0);
		}
		
		send(sockfd,&buffer, strlen(buffer),0);		
		
		//Here the client waits for a response from the server
		while(1)
		{ 
			while(read(sockfd ,buffer,sizeof(buffer)) != NULL)
			{
				printf("%s",buffer);
				break;
			}
			break;
			
		}
	}
	 
	
	close(sockfd);
	
	return 0;
}

void printMenu()
{
	printf(	"Please press a key:\n"	
			"[1] list content of current directory (ls)\n"
			"[2] print name of current directory (pwd)\n"
			"[3] change current directory (cd)\n"
			"[4] get file information\n"
			"[5] display file (cat)\n"
			"[?] this menu\n"
			"[q] quit\n");
}






