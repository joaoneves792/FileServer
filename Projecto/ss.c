#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>

#define BUFFERSIZE 1024
#define MAXFILESIZE 9 /*Number of decimal places for the max size of the downloaded file, 9 bytes long so we can store size info up to ~1GB plus '\0'*/

int main(int argc, char** argv) {

	int port = 59000;		/*By default connect on port 59000 */
	int i, j, k;
	int filesize = 0;

	int fd, newfd;
	struct sockaddr_in serveraddr, clientaddr;
	socklen_t clientlen;
	char buffer[BUFFERSIZE];  /*Small sized buffer for storing initial REQ message and REP nok message*/
	char* REPbuffer; /*Buffer used to load the requested file into and send the REP messsage*/

	char* filename;
	char* filepath;
	FILE* file;
	struct stat st;

	pid_t pid;

	/*Get the arguments*/
	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "-p"))
			port = atoi(argv[++i]);
	if (port <= 0) { /*If the port in the argument is 0 or if it is not a number*/
		printf("Invalid port number\n");
		return -1;
	}

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Error: socket()\n");
		exit(-1);
	}


	memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)port);

	if (bind(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) {
		printf("Error: bind()\n");
		close(fd);
		exit(-1);
	}


	if (listen(fd, 15) < 0) {
		printf("Error: listen()\n");
		close(fd);
		exit(-1);
	}

	clientlen = sizeof(clientaddr);

	/*Main loop - the server continues to accept new connections and launching child processes to attend to them*/
	while (1) {

		if ((newfd = accept(fd, (struct sockaddr*)&clientaddr, &clientlen)) < 0) {
			printf("Error: accept()\n");
			close(fd);
			exit(-1);
		}

		pid = fork();

		/*If its the parent close the newfd and return to the beginning of while*/
		if (pid) {
			close(newfd);
			continue;
		}


		/*The code below and until the end of while is only executed by the child process*/
		close(fd);

		/*Read and process the REQ message*/
		if (read(newfd, (void*)buffer, BUFFERSIZE) < 0) {
			printf("Error: read()\n");
			close(newfd);
			exit(-1);
		}
		strtok(buffer, " ");
		if (strcmp(buffer, "REQ")) {
			printf("Bad REQ message recieved\n");
			close(newfd);
			exit(-1);
		}
		filename = strtok(NULL, "\n");
		filepath = calloc(9 + strlen(filename), sizeof(char));
		sprintf(filepath, "contents/%s", filename);
		//printf("Content location: %s\n", filepath);

		printf("Request: %s from %s:%d\n", filename, inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);

		/*Open the requested file and prepare the REPbuffer*/
		file = fopen(filepath, "r");
		if (!file) {
			i = sprintf(buffer, "REP nok\n");
			write(newfd, (void*)buffer, i);
			close(newfd);
			exit(-1);
		}
		stat(filepath, &st);
		filesize = st.st_size;

		REPbuffer = (char*)malloc(sizeof(char)* (filesize+8+MAXFILESIZE));/*8 is the number of characters and spaces before the file*/

		i = sprintf(REPbuffer, "REP ok %d ", filesize); /*i is used as a reference to the end of the buffer*/

		/*Read the file into the REPbuffer*/
		while ((j = fread(REPbuffer+i, sizeof(char), filesize, file)))
			i +=j;
		fclose(file);

		i += sprintf(REPbuffer+i, "\n");

		k = 0;/*k is the number of bytes already written*/
		while (k < i) {
			j = write(newfd, (void*)(REPbuffer+k), i);
			if (j < 0) {
				printf("Error: write()\n");
				close(newfd);
				exit(-1);
			}
			k += j;
		}

		close(newfd);
		exit(0);
	}

	close(fd);

	return 0;
}

