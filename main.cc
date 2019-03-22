#include"thread_pool.h"
#include"epollServer.h"
#include"log.h"
#include"bloomfilter.h"
#include<iostream>
using namespace std;
int main(int argc,char *argv[]){

	LOG_INIT("weblog","log",6);
	LOG_DEBUG("log init over\n");//debug
	Bloomfilter::instance()->bloomfilterinit(0.01,10000,"redlist.txt");
	LOG_DEBUG("bloomfilter init over\n");//debug
	EpollServer *epoll=new EpollServer(80,4);
	epoll->init();	//创建监听套接字，创建线程池
	LOG_DEBUG("epoll init over\n");//debug
	printf("all init over , webserver start running\n");
	epoll->epoll();
	
}
