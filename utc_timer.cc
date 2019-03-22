#include"utc_timer.h"
#include<time.h>
#include<sys/time.h>
#include<stdio.h>

Utc_timer::Utc_timer()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	//set _sys_acc_sec, _sys_acc_min
	_sys_acc_sec=tv.tv_sec;
	_sys_acc_min=_sys_acc_sec/60;
	struct tm cur_tm;
	localtime_r((time_t*)&_sys_acc_sec,&cur_tm);
	year = cur_tm.tm_year + 1900;
        mon  = cur_tm.tm_mon + 1;
        day  = cur_tm.tm_mday;
        hour  = cur_tm.tm_hour;
        min  = cur_tm.tm_min;
        sec  = cur_tm.tm_sec;
        set_utc_fmt();
}

uint64_t Utc_timer::get_cur_time(int *p_msec)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	if(p_msec)
		*p_msec=tv.tv_usec/1000;//获得当前的毫秒数
	//如果和上次保存的时间没在同一秒内
	if((uint32_t)tv.tv_sec!=_sys_acc_sec)
	{
		sec=tv.tv_sec%60;
		_sys_acc_sec=tv.tv_sec;
		//如果和上次保存的时间没在同一分钟内
		if(_sys_acc_sec/60!=_sys_acc_min)
		{
			_sys_acc_min=_sys_acc_sec/60;
			struct tm cur_tm;
			 localtime_r((time_t*)&_sys_acc_sec, &cur_tm);
                	year = cur_tm.tm_year + 1900;
                	mon  = cur_tm.tm_mon + 1;
                	day  = cur_tm.tm_mday;
                	hour = cur_tm.tm_hour;
                	min  = cur_tm.tm_min;
                	set_utc_fmt();
		}
		else
		{
			set_utc_fmt_sec();	//只更改sec位
		}
	}
		return tv.tv_sec;	
}
void Utc_timer::set_utc_fmt()
{
	snprintf(utc_fmt, 20, "%d-%02d-%02d %02d:%02d:%02d", year, mon, day, hour, min, sec);

}

void Utc_timer::set_utc_fmt_sec()
    {
        snprintf(utc_fmt + 17, 3, "%02d", sec);
    }

