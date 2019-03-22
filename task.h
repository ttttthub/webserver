#ifndef _TASK_H_
#define _TASK_H_
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

#define MAX_BUFFER 2048 //buffer的最大字节数

class BaseTask
{
public:
	virtual void doit()=0;
};

class Task:public BaseTask
{
private:
	int client;
private:
	char method[255];
	char url[255];
	char path[512];
	char* query_string=NULL;
	char http_v[16];
public:
	Task(int fd):client(fd)
	{
	}	
	void doit();	
	void serve_file(int client, const char *filename,size_t filesize);
	void unimplemented(int client);
	int get_line(int sock, char *buf, int size); 
	void cat(int client, FILE *resource);
	void headers(int client, const char *filename,size_t filesize);
	void not_found(int client);
};

#endif
