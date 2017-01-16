#ifndef _USER_H_
#define _USER_H_

#define LSBUFFERSIZE 1024 /*size in bytes (a litle bigger than the max size of an AWT packet)*/
#define SSBUFFERSIZE 10000000/*size in bytes(should be big enough to recieve pictures,pdfs etc.)(It will resize by this amount if input is bigger than this)*/
#define SMALLBUFFERSIZE 5000 /*Size of the buffer used to recieve parts of the REP message as they are read*/
#define TIMEOUT_SEC 10 /*Timeout in seconds for UDP connections*/
#define MAXFILESIZE 10 /*Number of decimal places for the max size of the downloaded file ,10 bytes long so we can store size info up to ~1GB plus '\0'*/
#define RETRYCOUNT 5 /*Number of times to retry sending and recievieng a UDP message after a timeout*/

/*Structure used to return the information about the SServer on wich the selected topic is stored*/
typedef struct{
	char* IP;
	int port;
	char* topic;
}SServerInfo;

/*Structure used to return the information of the downloaded topic from the SServer*/
typedef struct{
	char* data;
	int size;
}DownloadedData;

#endif
