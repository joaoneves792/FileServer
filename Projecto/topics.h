typedef struct {
  char* ip;
  int port;
  char* name;
} Topic;

Topic *createTopic(const char *name, const char *ip, int port);
char *getSubject();
Topic *getTopic(int i);
int loadTopics();
