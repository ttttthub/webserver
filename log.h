#ifndef __LOG_H_
#define __LOG_H_

#include <stdio.h>
#include<string.h>
#include<stdint.h>
#include<pthread.h>
#include<time.h>
#include<sys/time.h>
#include<sys/types.h>
#include<stdlib.h>

#include"utc_timer.h"

extern pid_t gettid();

enum LOG_LEVEL
{
	FATAL=1,
	ERROR,
	INFO,
	DEBUG,
};

class cell_buff
{
public:
	cell_buff(uint32_t len):  
	status(0),	//cell_buff的状态 0表示有空闲，1表示满
	prev(NULL),
	next(NULL),
	total_len(len),	//一个缓冲区的大小
	used_len(0)	//已使用的大小
	{
		data=new char[len];
		if(!data)
		{
			fprintf(stderr,"no space to allocate data\n");
			exit(1);
		}
	}

	uint32_t avail_len() const {return total_len-used_len;}
	bool empty() const
	{
		if(used_len==0)
			return true;
		else
			return false;
	}

	void append(const char* log_line,uint32_t len) //将log_line写入到缓冲中
	{
		if(avail_len()<len)
			return;
		memcpy(data+used_len,log_line,len);
		used_len+=len;
		
	}

	void clear()
	{
		used_len=0;
		status=0;
	}
	
	void write_disk(FILE* fp)
	{
		uint32_t wt_len=fwrite(data,1,used_len,fp);
		if(wt_len!=used_len)
		{
			fprintf(stderr,"write log to disk error\n");
		}
	}
	
	bool status;
	cell_buff* prev;
	cell_buff* next;

private:
	cell_buff(const cell_buff&);	//禁止拷贝构造函数
	cell_buff& operator=(const cell_buff&);	//禁止赋值构造函数
	
	uint32_t total_len;
	uint32_t used_len;
	char* data;
};

class log
{
public:
	//单例
	static log* ins()
	{
		pthread_once(&_once,log::init);
		return _ins;
	}
	static void init()
	{
		while(!_ins)
		_ins=new log();
	}
	
	void path_init(const char* log_dir,const char* prog_name,int level);
	int get_level() const {return _level;}
	void write_disk();
	void try_append(const char* lvl,const char* format,...);

private:
	log();
	
	log(const log&);// 未定义，禁止
	const log& operator=(const log&);//未定以，禁止

	bool decis_file(int year,int mon,int day);

	int _buff_cnt;//缓存区块的数量
	
	cell_buff* producer_ptr;//生产者产生的日志往producer_ptr指向的缓存区缓存
	cell_buff* consumer_ptr;//消费者将consumer_ptr指向的缓存区写入到disk中
	
	cell_buff* last_buf;

	FILE* _fp;
	pid_t _pid;
	int _year,_mon,_day;
	int _log_cnt;
	
	char _prog_name[128];
	char _log_dir[512];

	bool _env_ok;
	int _level;
	uint64_t _lst_lts;

	Utc_timer _tm;
	
	static pthread_mutex_t _mutex;
	static pthread_cond_t _cond;
	
	static uint32_t _one_buf_len;

	//单例
	static log* _ins;
	static pthread_once_t _once;
};

void * be_thdo(void* args);

#define LOG_INIT(log_dir, prog_name, level) \
    do \
    { \
        log::ins()->path_init(log_dir, prog_name, level); \
        pthread_t tid; \
        pthread_create(&tid, NULL, be_thdo, NULL); \
        pthread_detach(tid); \
    } while (0)

#define LOG_FATAL(fmt, args...) \
    do \
    { \
        log::ins()->try_append("[FATAL]", "[%u]%s:%d(%s): " fmt "\n", \
            gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
    } while (0)

#define LOG_ERROR(fmt, args...) \
    do \
    { \
        if (log::ins()->get_level() >= ERROR) \
        { \
            log::ins()->try_append("[ERROR]", "[%u]%s:%d(%s): " fmt "\n", \
                gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)

#define LOG_INFO(fmt, args...) \
    do \
    { \
        if (log::ins()->get_level() >= INFO) \
        { \
            log::ins()->try_append("[INFO]", "[%u]%s:%d(%s): " fmt "\n", \
                    gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)

#define LOG_DEBUG(fmt, args...) \
    do \
    { \
        if (log::ins()->get_level() >= DEBUG) \
        { \
            log::ins()->try_append("[DEBUG]", "[%u]%s:%d(%s): " fmt "\n", \
                    gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)
#endif
