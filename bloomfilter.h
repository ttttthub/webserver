#ifndef _BLOOMFILTER_
#define _BLOOMFILTER_

#include<math.h>
#include<vector>
using namespace std;
unsigned int SDBMHash(const char *str);
unsigned int RSHash(const char *str);
unsigned int JSHash(const char *str);
unsigned int PJWHash(const char *str);
unsigned int APHash(const char *str);
unsigned int DJBHash(const char *str);
unsigned int ELFHash(const char *str);
unsigned int BKDRHash(const char *str);
class Bloomfilter{
public:
	void bloomfilterinit(double err_rate,int num,char* path);	//
    	~Bloomfilter();
    	bool is_contain(const char* str);	//
    	int hashnum();				//
        // double real_precision();	//
    	int sizeofpool();				//
	void filter_init();		//
	static Bloomfilter* instance();	//单例模式
private:
	static Bloomfilter* pbloom;
	void listinit();				//
	int  hashtable_init();			//
	int len; 
	char* mypath;			//
 //   	Bloomfilter(){};
    	double precision;
    	int *bitpool;		//
	int bitpoollen;		//
	int hashfuncnum;	//
	int samplenum;		//
	vector<unsigned int (*)(const char*)> hashtable;	//
};


#endif
