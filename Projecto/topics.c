#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "topics.h"

Topic *topics[30];
char subject[30];

Topic *createTopic(const char *name, const char *ip, int port) {
  Topic* topic = (Topic*) malloc(sizeof(Topic));
  topic->ip = (char*) calloc((strlen(ip)+1), sizeof(char));
  strcpy(topic->ip, ip);
  topic->port = port;
  topic->name = (char*) calloc((strlen(name)+1), sizeof(char));
  strcpy(topic->name, name);
  return topic;
}

char *getSubject() { return subject; }

Topic *getTopic(int i) { return topics[i]; }

int loadTopics() {
  char line[4096];
  int i = 0;
  FILE* file = fopen("topics.txt", "r");
  if (!file) {
    printf("Error: fopen(): cannot load available topics from file.\n");
    exit(-1);
  }

  //read the subject
  if (!fgets(line, 4096, file)) {
    printf("Error: fgets(): cannot get subject from file.\n");
    exit(-1);
  }
  strcpy(subject, line);
  subject[strlen(subject)-1] = 0;

  //read topics
  while (fgets(line, 4096, file)) {
    char *topic, *ip, *port;
    topic = strtok(line, " ");
    ip = strtok(NULL, " ");
    port = strtok(NULL, " ");
    if (topic && ip && port) topics[i++] = createTopic(topic, ip, atoi(port));
  }

  fclose(file);
  return i;
}
