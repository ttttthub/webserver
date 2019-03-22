#include<error.h>
#include<unistd.h>
#include<assert.h>
#include<stdarg.h>
#include<sys/stat.h>
#include<sys/syscall.h>

#include"log.h"

#define WAIT_TIME 1
#define LOG_USE_LIMIT (100*1024*1024)	//一个log文档的大小限制 100M
#define MEM_USE_LIMIT (1u*1024*1024*1024)	//缓存区的内存限制,1G
#define LOG_LEN_LIMIT (4*1024)		//一条日志的长度限制,4k
#define RELOG_THRESOLD 5
pid_t gettid()
{
	return syscall(__NR_gettid);
}

pthread_mutex_t log::_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t log::_cond=PTHREAD_COND_INITIALIZER;
pthread_once_t log::_once=PTHREAD_ONCE_INIT;

log* log::_ins=NULL;
uint32_t log::_one_buf_len=30*1024*1024;	//30MB

log::log():
	_buff_cnt(3),
	producer_ptr(NULL),
	consumer_ptr(NULL),
	_fp(NULL),
	_log_cnt(0),
	_env_ok(false),
	_level(INFO),
	_lst_lts(0),
	_tm()
{
	//建立缓存区的双向链表
	cell_buff* head=new cell_buff(_one_buf_len);
	if(!head)
	{
		fprintf(stdout,"no space to allocate cell_buff\n");
		exit(1);
	}	
	cell_buff* cur;
	cell_buff* prev=head;
	for(int i=1;i!=_buff_cnt;i++)
	{
		cur= new cell_buff(_one_buf_len);
		if(!cur)
		{	
			fprintf(stdout,"no space to allocate cell_buff\n");
			exit(1);
		}
		prev->next=cur;
		cur->prev=prev;
		prev=cur;
	}
	//首尾相接
	prev->next=head;
	head->prev=prev;

	producer_ptr=head;
	consumer_ptr=head;
	
	_pid=getpid();
}
// mkdir log_dir   创建成功_env_ok为true，否则为false
void log::path_init(const char* log_dir,const char* prog_name,int level)
{
	pthread_mutex_lock(&_mutex);

	strncpy(_log_dir,log_dir,512);
	strncpy(_prog_name,prog_name,128);

	mkdir(_log_dir,0777);
	//查看是否存在此目录，目录下是否允许创建文件
	if(access(_log_dir,F_OK|W_OK)==-1)
	{
		fprintf(stdout,"%s create failure",_log_dir);
	}
	else
	{
		_env_ok=true;
	}
	
	_level=level;

	pthread_mutex_unlock(&_mutex);
}

void log::write_disk()
{
	while(true)
	{
		pthread_mutex_lock(&_mutex);
		
		if(consumer_ptr->status==0)
		{
			struct timespec tsp;
			struct timeval now;
			gettimeofday(&now,NULL);
			tsp.tv_sec=now.tv_sec;
			tsp.tv_sec+=WAIT_TIME;//wait for 1 second
			tsp.tv_nsec = now.tv_usec * 1000;//nanoseconds
			pthread_cond_timedwait(&_cond,&_mutex,&tsp);
		}		
		if(consumer_ptr->empty())
		{
			//printf("empty\n");
			pthread_mutex_unlock(&_mutex);
			continue;
		}
		
		if(consumer_ptr->status==0)
		{
			assert(consumer_ptr==producer_ptr);
			consumer_ptr->status=1;	 //消费者指针指向cell_buff置1
		//	printf("zhi 1\n");
			producer_ptr=producer_ptr->next;//生产者指针后移
		}

		int year=_tm.year,mon=_tm.mon,day=_tm.day;
		
		pthread_mutex_unlock(&_mutex);
		
		//选择写到哪个文件里
		if(!decis_file(year,mon,day))	//写一次传一次日期时间，选择写到哪个log文档
			continue;
		//write,只写status为1的cell_buff
		consumer_ptr->write_disk(_fp);
		fflush(_fp);


		//printf("xie wan  l\n");
		pthread_mutex_lock(&_mutex);
		consumer_ptr->clear();
		consumer_ptr=consumer_ptr->next;
		pthread_mutex_unlock(&_mutex);
		
	}
}

void log::try_append(const char* lvl,const char* format,...)
{
	int ms;
	uint64_t curr_sec=_tm.get_cur_time(&ms);
	
	//上次出错到现在小于RELOG_THRESOLD s,放弃本次写入直接返回
	if(_lst_lts&&curr_sec-_lst_lts<RELOG_THRESOLD)
		return;
	
	char log_line[LOG_LEN_LIMIT];
	
	int prev_len=snprintf(log_line,LOG_LEN_LIMIT,"%s[%s.%03d]",lvl,_tm.utc_fmt,ms);
	
	va_list arg_ptr;
	va_start(arg_ptr,format);
	int main_len=vsnprintf(log_line+prev_len,LOG_LEN_LIMIT-prev_len,format,arg_ptr);
	va_end(arg_ptr);

	uint32_t len=prev_len+main_len;
	
	_lst_lts=0;
	bool tell_back=false;
	
	pthread_mutex_lock(&_mutex);
	if (producer_ptr->status == 0&& producer_ptr->avail_len() >= len)
    	{
        	producer_ptr->append(log_line, len);
    	}
	else
	{
		if(producer_ptr->status==0)
		{
			producer_ptr->status=1;
			cell_buff* next_buff=producer_ptr->next;
			tell_back=true;		
			//next_buff正在写磁盘，即缓存区双向链表已满
			if(next_buff->status==1)
			{
				//缓存区的大小超过了限制
				if(_one_buf_len*(_buff_cnt+1)>MEM_USE_LIMIT)
				{
					fprintf(stdout,"no more space can use\n");
					producer_ptr=next_buff;
					_lst_lts=curr_sec;		//记录出错时间
				}
				//重新申请一个cell_buff
				else
				{
					
					cell_buff* new_buff=new cell_buff(_one_buf_len);
					_buff_cnt++;
					new_buff->prev=producer_ptr;
					producer_ptr=new_buff;
					new_buff->next=next_buff;
					next_buff->prev=new_buff;
					producer_ptr=new_buff;
				
				}
			}
			else
			{
				producer_ptr=next_buff;
			}
			//没有出错，正常写缓存
			if(!_lst_lts)
				{
					producer_ptr->append(log_line,len);
				//	 printf("写缓存正常\n");
				}
		}
		else	//缓存区满，丢弃将要输出的日志	
		{
			_lst_lts=curr_sec;
		}
	}
//	printf("append succellful\n");
//	printf("%s\n",log_line);
	pthread_mutex_unlock(&_mutex);
	if(tell_back)
	{
		pthread_cond_signal(&_cond);
	}
}

bool log::decis_file(int year,int mon,int day)
{
	if(!_env_ok)
	{
		return false;	
	}

	//第一次给_prog_name写日志时，需创建文件
	if(!_fp)
	{
		_year=year,_mon=mon,_day=day;
		char log_path[1024]={};
		sprintf(log_path,"%s/%s.%d%02d%02d.%u.log",_log_dir,_prog_name,_year,_mon,_day,_pid);
		_fp=fopen(log_path,"w");
		//printf("%s\n",log_path);
		if(_fp)
			_log_cnt++;
	}
	//新的一天，新的一个记录log的文件
	else if(_day!=day)
	{
		fclose(_fp);
		char log_path[1024]={};
		_year=year,_mon=mon,_day=day;
		sprintf(log_path,"%s/%s.%d%02d%02d.%u.log",_log_dir,_prog_name,_year,_mon,_day,_pid);
		_fp=fopen(log_path,"w");
		if(_fp)
			_log_cnt=1;
	}
	//log文件过大，重新打开一个log文档，并修改前几个文档的名字
	else if(ftell(_fp)>=LOG_USE_LIMIT)
	{
		fclose(_fp);
		char old_path[1024]={};
		char new_path[1024]={};
		//修改文档名 mv xxx.log.[i]  xxx.log.[i+1]
		for(int i=_log_cnt-1;i>0;--i)
		{
			sprintf(old_path,"%s/%s.%d%02d%02d.%u.log.%d",_log_dir,_prog_name,_year,_mon,_day,_pid,i);
			sprintf(new_path,"%s/%s.%d%02d%02d.%u.log.%d",_log_dir,_prog_name,_year,_mon,_day,_pid,i+1);
			rename(old_path,new_path);
		}
	// mv xxx.log xxx.log.1
	sprintf(old_path,"%s/%s.%d%02d%02d.%u.log",_log_dir,_prog_name,_year,_mon,_day,_pid);	
	sprintf(new_path,"%s/%s.%d%02d%02d.%u.log.1",_log_dir,_prog_name,_year,_mon,_day,_pid);
	rename(old_path,new_path);	
	_fp=fopen(old_path,"w");
	if(_fp)
		_log_cnt++;
	}
	return _fp!=NULL;
}

void* be_thdo(void* args)
{
	log::ins()->write_disk();
	return NULL;
}
