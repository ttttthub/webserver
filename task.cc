#include"task.h"
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

#include"log.h"
#include"bloomfilter.h"
void Task::doit()
{
	char buf[1024];
	int numchars;
//	char method[255];
//	char url[255];
//	char path[512];
	size_t i,j;
	struct stat st;
	
	numchars=get_line(client,buf,sizeof(buf));
	i=0;j=0;
	
	//得到该请求报文的请求方法
	while(!ISspace(buf[j])&&(i<sizeof(method)-1))	
	{
		method[i]=buf[j];
		i++;
		j++;
	}
	method[i]= '\0';

	//该web服务器目前仅支持GET,POST待做
	if(strcasecmp(method,"GET")&&strcasecmp(method,"POST"))
	{
		LOG_INFO("Unrealized Method is %s\n",method);
		unimplemented(client);
		close(client);
		return;	
	}

	//得到该请求报文中的url
	i=0;
	while(ISspace(buf[j])&&(j<sizeof(buf)))
	j++;
	while(!ISspace(buf[j])&&(i<sizeof(url)-1)&&j<sizeof(buf))
	{
		url[i]=buf[j];
		i++;
		j++;		
	}
	url[i]='\0';

	//printf("url is %s\n",url);   //debug
	
	//http版本获得
	i=0;	
	while(ISspace(buf[j])&&(j<sizeof(buf)))
	j++;	
	while(!ISspace(buf[j])&&(i<sizeof(http_v)-1)&&j<sizeof(buf))
	{
		http_v[i]=buf[j];
		i++;
		j++;		
	}
	http_v[i]='\0';

	//LOG_INFO("http_v is %s\n",http_v);
	//请求参数获得
	if(strcasecmp(method,"GET")==0)
	{
		query_string=url;
		while((*query_string!='?')&&(*query_string!='\0'))
		query_string++;
		if(*query_string=='?')
		{
			*query_string='\0';
			query_string++;
		}
	
	}
			
	if(strcasecmp(url,"/favicon.ico")==0)
	{
		LOG_DEBUG("request favicon.ico error, discard this message"); //debug	
		while((numchars>0)&&strcmp("\n",buf))	
			{
				numchars=get_line(client,buf,sizeof(buf));
				
			}
			close(client);
			return;
	}

	sprintf(path,"htdocs%s",url);
	

	if(path[strlen(path)-1]=='/')
		strcat(path,"index.html");
	if(stat(path,&st)==-1)	//该文件没有找到
	{
		while((numchars>0)&&strcmp("\n",buf))	
			{
				numchars=get_line(client,buf,sizeof(buf)); 
				not_found(client);
			}

	}
	else
	{
		if((st.st_mode&S_IFMT)==S_IFDIR)
			strcat(path,"index.html");
		
			LOG_INFO("path: %s\n",path);

		
			if(strcasecmp(path,"htdocs/index1.html")==0)
                        {
                                query_string+=6;//跳过开头再开始比对
                                LOG_INFO("input is  %s\n",query_string);
                                if(!Bloomfilter::instance()->is_contain(query_string))
                                        {
                                                not_found(client);
                                                close(client);
                                                return;
                                        }


                        }

	
			serve_file(client,path,st.st_size);
			LOG_INFO("file has already sent\n");			
	}
	close(client);		
}

void Task::serve_file(int client,const char* filename,size_t file_size)
{
	FILE* resource=NULL;
	int numchars=1;
	char buf[1024];
	
	buf[0]='A';
	buf[1]='\0';
	
	while((numchars>0)&&strcmp("\n",buf))
	numchars=get_line(client,buf,sizeof(buf));  //read and discard headers

	resource = fopen(filename, "r");
 	if (resource == NULL)
  	not_found(client);
 	else
 	{
  	headers(client, filename,file_size);
  	cat(client, resource);
 	}
 	fclose(resource);
}
void Task::unimplemented(int client)
{
	char buf[1024];

 	sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, SERVER_STRING);
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "Content-Type: text/html\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "</TITLE></HEAD>\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "</BODY></HTML>\r\n");
 	send(client, buf, strlen(buf), 0);
}
int Task::get_line(int sock, char *buf, int size)
{
 	int i = 0;
 	char c = '\0';
 	int n;

 	while ((i < size - 1) && (c != '\n'))
 	{
  	n = recv(sock, &c, 1, 0);
  	/* DEBUG printf("%02X\n", c); */
  	if (n > 0)
  		{
   			if (c == '\r')
   			{
    				n = recv(sock, &c, 1, MSG_PEEK);
    				/* DEBUG printf("%02X\n", c); */
    				if ((n > 0) && (c == '\n'))
     				recv(sock, &c, 1, 0);
    				else
     				c = '\n';
   			}
   			buf[i] = c;
   			i++;
  		}
  	else
   	c = '\n';
 	}
 	buf[i] = '\0';

 	return(i);
	
}
void Task::cat(int client, FILE *resource)
{
 	char buf[10];

 	fgets(buf, sizeof(buf), resource);
 	while (!feof(resource))
 	{
  	send(client, buf, strlen(buf), 0);
 	fgets(buf,sizeof(buf), resource);
 	}
}
void Task::headers(int client, const char *filename,size_t filesize)
{
 	char buf[1024];
 	(void)filename;  /* could use filename to determine file type */

 	strcpy(buf, "HTTP/1.1 200 OK\r\n");
 	send(client, buf, strlen(buf), 0);
 	strcpy(buf, SERVER_STRING);
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "Content-Type: text/html\r\n");
 	send(client, buf, strlen(buf), 0);	
 	sprintf(buf, "Content-Length: %zu\r\n",filesize);
 	send(client, buf, strlen(buf), 0);
 	strcpy(buf, "\r\n");
 	send(client, buf, strlen(buf), 0);
}
void Task::not_found(int client)
{
 	char buf[1024];
 	sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, SERVER_STRING);
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "Content-Type: text/html\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
 	send(client, buf, strlen(buf), 0);
  	sprintf(buf, "your request because the resource specified\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "is unavailable or nonexistent.\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "</BODY></HTML>\r\n");
 	send(client, buf, strlen(buf), 0);
}

