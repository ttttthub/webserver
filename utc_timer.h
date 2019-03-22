#ifndef _UTCTIMER_H
#define _UTCTIMER_H

#include<iostream>
#include<string.h>
#include<sys/time.h>
#include<time.h>
#include<stdint.h>

class Utc_timer
{
public:
	Utc_timer();
	uint64_t get_cur_time(int* p_msec=NULL);  //？参数
	int year;
	int mon;
	int day;
	int hour;
	int min;
	int sec;
	char utc_fmt[20];	//将year,mon,day,hour,min,sec格式化保存在utc_fmt中，以备输出
private:
	void set_utc_fmt();	
	void set_utc_fmt_sec();	//同一分钟时，只更改utc_fmt中的sec值
	uint64_t _sys_acc_min;
	uint64_t _sys_acc_sec;
};
#endif
