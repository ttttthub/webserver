#ifndef _EPOLL_SERVER_H_
#define _EPOLL_SERVER_H

#include<sys/socket.h>
#include<sys/types.h>
#include<stdio.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/wait.h>
#include<sys/epoll.h>

#include"thread_pool.h"
#include"task.h"
#include"log.h"
//#define MAX_BUFFER 2048  //BUffer的最大字节
#define MAX_EVENT 1024 //epoll_events的最大个数
class EpollServer
{
private:
	bool is_stop;	//是否停止epoll_wait的标志
	int threadnum;	//线程数目
	int sockfd;	//监听的fd
	int port;	//端口
	int epollfd;	//Epoll的fd
	threadpool<BaseTask> *pool;	//线程池的指针
	epoll_event events[MAX_EVENT];	//epoll的events数组
	struct sockaddr_in bindAddr;	//绑定的sockaddr
public:
	//构造函数
	EpollServer(int ports,int thread):is_stop(false),threadnum(thread)
	,port(ports),pool(NULL)
	{
	}

	~EpollServer()	//析构
	{
		delete pool;
	}	
	
	void init();
	
	void epoll();
	
	static int setnonblocking(int fd)	//将fd设置成非阻塞
	{
		int old_option=fcntl(fd,F_GETFL);
		int new_option=old_option|O_NONBLOCK;
		fcntl(fd,F_SETFL,new_option);
		return old_option;
	}

	static void addfd(int epollfd,int sockfd,bool oneshot) //向epoll中添加fd
	{//oneshot表示是否设置成同一时刻，只有一个线程访问fd，数据的读取都在主线程中，所以调用设置成false
		epoll_event event;
		event.data.fd=sockfd;
		event.events=EPOLLIN|EPOLLET;
		if(oneshot)
		{
			event.events|=EPOLLONESHOT;
		}		
		epoll_ctl(epollfd,EPOLL_CTL_ADD,sockfd,&event);	//添加fd;
		EpollServer::setnonblocking(sockfd);
	}
};

void EpollServer::init()	//EpollServe的初始化
{
	bzero(&bindAddr,sizeof(bindAddr));
	bindAddr.sin_family=AF_INET;
	bindAddr.sin_port=htons(port);
	bindAddr.sin_addr.s_addr=htonl(INADDR_ANY);
	//创建socket
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0)
	{
		printf("EpollServe socket init error\n");
		return;
	}
	int ret=bind(sockfd,(struct sockaddr*)&bindAddr,sizeof(bindAddr));
	if(ret<0)
	{
		printf("EpollServe socket bind error\n");
		return;
	}

	ret=listen(sockfd,50);
	if(ret<0)
	{	
		printf("EpollServe socket listen error\n");
		return;
	}	

	epollfd=epoll_create(1024);
	if(epollfd<0)
	{	
		printf("EpollServe epoll_create  error\n");
		return;
	}
	
	pool=new threadpool<BaseTask>(threadnum);	//创建线程池
}

void EpollServer::epoll()
{
	pool->start();	//线程池启动
	
	EpollServer::addfd(epollfd,sockfd,false);
	while(!is_stop)
	{//调用epoll_wait
		int ret=epoll_wait(epollfd,events,MAX_EVENT,-1);
		if(ret<0) //出错处理
		{
			printf("epoll_wait error\n");
				continue;
		}
		
		for(int i=0;i<ret;i++)
		{
			int fd=events[i].data.fd;
			if(fd==sockfd)	//监听套接字可读
			{
				struct sockaddr_in clientAddr;
				socklen_t len=sizeof(clientAddr);
				int confd=accept(sockfd,(struct sockaddr*)&clientAddr,&len);
				 struct sockaddr_in *sock = ( struct sockaddr_in*)&clientAddr;

				int port = ntohs(sock->sin_port);
				struct in_addr in=sock->sin_addr;
				char str[16];
				inet_ntop(AF_INET,&in, str, sizeof(str));
				LOG_INFO("ip：port %s:%d\n",str,port);
				EpollServer::addfd(epollfd,confd,false);
			}	
			else if(events[i].events&EPOLLIN)//某个fd上有数据可读
			{
			
				/*
				char buffer[MAX_BUFFER];
		readagain:	memset(buffer,0,sizeof(buffer));
				int ret=read(fd,buffer,MAX_BUFFER-1);
				if(ret==0)	//某个fd关闭了连接，从Epoll中删除并关闭fd			
				{
					epoll_event ev;
					ev.events=EPOLLIN;
					ev.data.fd=fd;
					epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,&ev);
					shutdown(fd,SHUT_RDWR);
					printf("%d logout\n",fd);
					continue;
				}
				else if(ret<0)	//读取出错，尝试再次读取
				{
					if(errno==EAGAIN)
					{
						printf(" read error!read again\n");
						goto readagain;
						break;
                                        }	
				}
				else //成功读取，向线程池中添加任务
				{
					BaseTask *task =new Task(buffer,fd);
					pool->append_task(task);
				}				
					*/
				BaseTask *task=new Task(fd);
				pool->append_task(task);
			}

		}
		

	}
		close(sockfd);	
		pool->stop();
		
}

#endif
