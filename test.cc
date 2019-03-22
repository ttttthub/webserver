#include"bloomfilter.h"
#include"log.h"
#include<iostream>
#include<stdio.h>
#include<fstream>
#include<string>
#include"utc_timer.h"
#include"unistd.h"
using namespace std;

int getcurtime()
{
	struct timeval tv;
        gettimeofday(&tv,NULL);//获取1970-1-1到现在的时间结果保存到tv中
        uint64_t sec=tv.tv_sec;
        uint64_t min=tv.tv_sec/60;
        struct tm cur_tm;//保存转换后的时间结果
        localtime_r((time_t*)&sec,&cur_tm);
        //char cur_time[20];
        //snprintf(cur_time,20,"%d-%02d-%02d %02d:%02d:%02d",cur_tm.tm_year+1900,cur_tm.tm_mon+1,cur_tm.tm_mday,cur_tm.tm_hour,cur_tm.tm_min,cur_tm.tm_sec);
        //printf("current time is %s\n",cur_time);//打印当前时间
	//cout<<cur_time;

}


int main()
{
	/*
	struct timeval tv1,tv2;	
        gettimeofday(&tv1,NULL);//获取1970-1-1到现在的时间结果保存到tv中
	Utc_timer timer;
	int *p;
	for(int i=0;i!=10000000;i++)
	{
	//	timer.get_cur_time(p);
		//cout<<timer.utc_fmt;
		getcurtime();
}

        gettimeofday(&tv2,NULL);//获取1970-1-1到现在的时间结果保存到tv中
	cout<<tv2.tv_sec-tv1.tv_sec<<endl;
	*/
	
	struct timeval tv1,tv2;	
        gettimeofday(&tv1,NULL);//获取1970-1-1到现在的时间结果保存到tv中
	
	LOG_INIT("log","log",3);
	for(int i=0;i!=10000000;i++)	
	LOG_FATAL("hello world");
	
        gettimeofday(&tv2,NULL);//获取1970-1-1到现在的时间结果保存到tv中
	
	//FILE* fp=fopen("log.txt","w");
	cout<<tv2.tv_sec-tv1.tv_sec<<endl;
}
