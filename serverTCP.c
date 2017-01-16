#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define PORT 58000


int main(int argc, char* argv){
	                                           
	int fd, newfd;                                    
	struct hostent *hostptr;                   
	struct sockaddr_in serveraddr, clientaddr; 
	int clientlen; 
	char buffer[1024];  
	                          

	fd = socket(AF_INET, SOCK_STREAM, 0);
	
	memset((void*)&serveraddr,(int)'\0', sizeof(serveraddr));
	
        serveraddr.sin_family=AF_INET;
	serveraddr.sin_addr.s_addr=htonl(INADDR_ANY);
        serveraddr.sin_port=htons((u_short)PORT);

        if (bind(fd,(struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) 
		printf("Error: bind()\n");
	else	
		printf("Servidor a correr\n");
	
        fflush( stdout);
	listen(fd, 5);
        
	clientlen = sizeof(clientaddr);
	newfd = accept(fd, (struct sockaddr*)&clientaddr, &clientlen);
	
	while (read(newfd, (void*)buffer,1024)) {
		printf("\n%s\n", buffer);
		write(newfd, (void*)buffer, strlen(buffer));
	}


	close(newfd);
	close(fd);
}

