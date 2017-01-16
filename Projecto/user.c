#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>

#include "user.h"

extern int errno;

/*sendAndRecieveUDP:	sends the contents of buffer in a packet, recieves a packet and puts its contents in the buffer
			If the socket times out on recvfrom it sends a new packet with the contents of buffer*/
void sendAndRecieveUDP(int fd, char* buffer, struct sockaddr_in serveraddr, socklen_t addrlen) {
	int i;
	for (i = 0; i < RETRYCOUNT; i++) {
		if (sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*)&serveraddr,  addrlen) < strlen(buffer)) {
			printf("Error: sendto(), ERRNO = %d\n", errno);
			close(fd);
			exit(-1);
		}
		if (recvfrom(fd, buffer, LSBUFFERSIZE, 0, (struct sockaddr*)&serveraddr,  &addrlen) < 0)
			if (errno == EAGAIN || errno == EWOULDBLOCK) { /*If its a timeout*/
				if (i == RETRYCOUNT-1) {
					printf("Request timed out. (Server is probably down)\n");
					close(fd);
					exit(-1);
				}else
					printf("Request timed out, trying again... (Press Ctrl-C to quit)\n");
			}else{
				printf("Error: recvfrom(), ERRNO = %d\n", errno);
				close(fd);
				exit(-1);
			}
		else
			break;
	}
}

SServerInfo* connectLS(char* hostname, int port) {

	int fd;
	socklen_t addrlen;
	struct hostent *hostptr;
	struct sockaddr_in serveraddr;
	struct timeval timeout;

	char buffer[LSBUFFERSIZE];

	char* subject;
	char* topic;
	int numOfTopics, i, selectedTopic;
	char** topics;

	char* SSIPAdress;
	int   SSPort;
	SServerInfo* ssInfo;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Error: socket()\n");
		exit(-1);
	}

	if (!(hostptr = gethostbyname(hostname))) {
		printf("Error: gethostbyname()\n");
		close(fd);
		exit(-1);
	}
	memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = ((struct in_addr*) (hostptr->h_addr_list[0]))->s_addr;
	serveraddr.sin_port = htons((unsigned short)port);

	addrlen = sizeof(serveraddr);

	/*Set the timeout for this socket*/
	timeout.tv_sec = TIMEOUT_SEC;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));


	/*Prepare the buffer with the REQUEST TOPIC message*/
	sprintf(buffer, "RQT\n");

	/*Send the RQT message and recieve the AWT message*/
	sendAndRecieveUDP(fd, buffer, serveraddr, addrlen);

	/*Split the AWT message into subject, number of topics and an Array with the topics*/
	if (strcmp("AWT", strtok(buffer, " "))) {
		printf("ERROR: Recieved package doesn't begin with AWT \n");
		close(fd);
		exit(-1);
	}

	subject = strtok(NULL, " ");
	numOfTopics = atoi(strtok(NULL, " "));
	if (numOfTopics == 0) { /*If atoi cant find a valid integer to convert it returns 0*/
		printf("No topics on this server or message is malformed\n");
		close(fd);
		exit(-1);
	}


	topics = (char**)calloc(numOfTopics, sizeof(char*)); /* Allocate an array for the topics*/
	for (i = 1; i < numOfTopics; i++)
		topics[i-1] = strtok(NULL, " ");
	topics[numOfTopics-1] = strtok(NULL, "\n"); /*The last topic ends with \n after that its all garbage*/


	/*Print the subject and the list of topics*/
	printf("\n%s:\n", subject);
	for (i = 1; i <= numOfTopics; i++)
		printf("%d-%s\n", i, topics[i-1]);
	printf(">");

	free(topics); /* Free the Array of topics, we dont need that anymore */

	/*Ask the user for the topic he wants*/
	while (scanf("%d", &selectedTopic) != 1 || selectedTopic > numOfTopics || selectedTopic < 1) {
		if (selectedTopic == 0) {
			close(fd);
			exit(0);
		}
		getchar();/*consume the newline*/
		printf("Invalid input. Please try again. \n>");
	}

	/*Prepare the buffer with the REQUEST CONTENT message*/
	sprintf(buffer, "RQC %d\n", selectedTopic);

	/*Send the RQC message and recieve the AWC message*/
	sendAndRecieveUDP(fd, buffer, serveraddr, addrlen);

	/*Split the AWC message into topic, IP adress and port of the StorageServer*/
	if (strcmp("AWC", strtok(buffer, " "))) {
		printf("ERROR: Recieved package doesn't begin with AWC \n");
		close(fd);
		exit(-1);
	}

	topic = strtok(NULL, " ");
	SSIPAdress = strtok(NULL, " ");
	SSPort = atoi(strtok(NULL, "\n")); /*The last topic ends with \n after that its all garbage*/
	if (SSPort == 0) {
		printf("Invalid port (0) or message is malformed\n");
		close(fd);
		exit(-1);
	}


	printf("%s %s:%d\n", topic, SSIPAdress, SSPort);

	close(fd);

	/*Prepare the SServerInfo struct to be returned with all the information needed to connect to the Storage Server*/
	ssInfo = (SServerInfo*)malloc(sizeof(SServerInfo));
	ssInfo->IP = (char*)calloc(strlen(SSIPAdress)+1, sizeof(char));
	strcpy(ssInfo->IP, SSIPAdress);
	ssInfo->port = SSPort;
	ssInfo->topic = (char*)calloc(strlen(topic)+1, sizeof(char));
	strcpy(ssInfo->topic, topic);

	return ssInfo;

}

DownloadedData* connectSS(char* serverIP, int port, char* topic) {

	int fd, i, j, k, size;
	struct hostent *hostptr;
	struct sockaddr_in serveraddr;
	int bufferSize = SSBUFFERSIZE;/*initial size of the buffer, if the recieved message exceeds this it will be extended with +SSBUFFERSIZE*/
	char* buffer;
	char* data;
	char smallBuffer[SMALLBUFFERSIZE];/*Small buffer used to store parts of the REP message as they are read*/
	char tmp[MAXFILESIZE]; /*Temporary string to store parts of the REP message*/
	DownloadedData* downloadedData;


	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Error: socket()\n");
		exit(-1);
	}

	if (!(hostptr = gethostbyname(serverIP))) {
		printf("Error: gethostbyname()\n");
		close(fd);
		exit(-1);
	}
	memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = ((struct in_addr*) (hostptr->h_addr_list[0]))->s_addr;
	serveraddr.sin_port = htons((unsigned short)port);

	if (connect(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) {
		printf("Error: connect(). ERRNO = %d\n", errno);
		close(fd);
		exit(-1);
	}

	/*Alloc the memory for the buffer(This buffer must be large enough to recieve files from the server, therefore it shouldnt be created in the stack)*/
	/*Also its size must be constant because we dont know the size of the message before we recieve the whole thing*/
	buffer = (char*)calloc(bufferSize, sizeof(char));

	/*Prepare the buffer with the REQUEST CONTENT message*/
	sprintf(buffer, "REQ %s\n", topic);

	/*Send the REQ message*/
	if (write(fd, buffer, strlen(buffer)) < strlen(buffer)) { /*write() returns the number of bytes writen or -1 if it fails*/
		printf("Error: write(). ERRNO = %d\n", errno);
		close(fd);
		exit(-1);
	}
	/*Keep reading until we reach the end of the transmission*/
	j = 0;/*j is used to indicate the next pos to write in the buffer*/
	while ((i = read(fd, smallBuffer, SMALLBUFFERSIZE))) {
		if (i < 0) {
			printf("Error: read(). ERRNO = %d\n", errno);
			close(fd);
			exit(-1);
		}
		if (j+i > bufferSize) {/*If the message exceeds the size of the buffer we extend it with an extra SSBUFFERSIZE size*/
			buffer = (char*)realloc(buffer, bufferSize+SSBUFFERSIZE);
			bufferSize += SSBUFFERSIZE;
		}
		for (k = 0; k < i; k++)
			buffer[j+k] = smallBuffer[k];
		j += i;
	}

	close(fd);

	/*Split the REP answer*/
	/*We cant use strtok here because it modifies the buffer!*/
	for (i = 0; i < 3; i++)
		tmp[i] = buffer[i];
	tmp[3] = '\0';
	if (strcmp("REP", tmp)) {
		printf("ERROR: Recieved message doesn't begin with REP\n");
		exit(-1);
	}

	/*check if the REP is ok or nok*/
	if ('n' == buffer[4]) {
		printf("ERROR: Storage Server replied NOK\n");
		exit(-1);
	}

	/*Get the size of the contents*/
	i = 0;
	while (buffer[7+i] != ' ') {/*7 is the first position of the size in the buffer*/
		if (i >= MAXFILESIZE) {
			printf("Recieved file exceeds maximum size.\n");
			exit(-1);
		}
		tmp[i] = buffer[7+i];
		i++;
	}
	tmp[i] = '\0';
	size = atoi(tmp);
	if (size == 0) {
		printf("No data on the message (size = 0 bytes) or message is malformed\n");
		exit(-1);
	}

	/*Create the data array and copy the content of the message to that new array*/
	data = (char*)calloc(size, sizeof(char));
	for (j = 0; j < size; j++)
		data[j] = buffer[j+7+i+1];/*7 is the first pos of the size, 7+i+1 is the beginig of the content*/
	if (buffer[j+7+i+1] != '\n') {
		printf("Message not terminated with \\n or size is wrong\n");
		exit(-1);
	}

	free(buffer);

	downloadedData = (DownloadedData*)malloc(sizeof(DownloadedData));
	downloadedData->data = data;
	downloadedData->size = size;

	return downloadedData;

}

void writeToFile(char* fileName, char* data, int size) {
	/*Write the data into a file named after the topic*/
	FILE* file;
	file = fopen(fileName, "w+");
	if (fwrite(data, 1, size, file) < size)
		printf("There was an error while writing the downloaded data to the file.\n");
	fclose(file);
}

int main(int argc, char** argv) {
	char* serverHostName = "localhost";	/*By default connect to localhost*/
	int LServerPort = 58050;		/*By default connecto on port 58000 + group number*/
	int i;
	SServerInfo*	ssInfo;
	DownloadedData* data;

	/*Get the arguments*/
	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "-n"))
			serverHostName = argv[i+1];
		else if (!strcmp(argv[i], "-p"))
			LServerPort = atoi(argv[i+1]);
	if (LServerPort == 0) { /*If the port in the argument is 0 or if it is not a number*/
		printf("Invalid port number\n");
		return -1;
	}


	/*Connect to the LS and get the info required to connect to the SS*/
	ssInfo = connectLS(serverHostName, LServerPort);

	/*Connect to the SS and download the selected contents*/
	data = connectSS(ssInfo->IP, ssInfo->port, ssInfo->topic);

	/*Write the downloaded data to a file*/
	writeToFile(ssInfo->topic, data->data, data->size);

	/*Free the structure and array with the downloaded data*/
	free(data->data);
	free(data);
	/*Free the Storage Server Info structure (created inside connectLS)*/
	free(ssInfo->IP);
	free(ssInfo->topic);
	free(ssInfo);


	return 0;
}


