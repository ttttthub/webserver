#include<iostream>
#include<string>
#include<math.h>
#include<stdio.h>
#include"bloomfilter.h"
#include<fstream>
double lg2(double n)
{
	return log(n)/log(2);
}


using namespace std;

//几个哈希函数
unsigned int SDBMHash(const char *str)
{
    unsigned int hash = 0;

    while (*str)
    {
      hash = (*str++) + (hash << 6) + (hash << 16) - hash;
     }

      return (hash & 0x7FFFFFFF);
}

unsigned int RSHash(const char *str)
{
    unsigned int b = 378551;
    unsigned int a = 63689;
    unsigned int hash = 0;

    while (*str)
    {
        hash = hash * a + (*str++);
        a *= b;
    }

    return (hash & 0x7FFFFFFF);
}

unsigned int JSHash(const char *str)
{
    unsigned int hash = 1315423911;

    while (*str)
    {
        hash ^= ((hash << 5) + (*str++) + (hash >> 2));
    }

    return (hash & 0x7FFFFFFF);
}

unsigned int PJWHash(const char *str)
{
    unsigned int BitsInUnignedInt = (unsigned int)(sizeof(unsigned int) * 8);
    unsigned int ThreeQuarters    = (unsigned int)((BitsInUnignedInt  * 3) / 4);
    unsigned int OneEighth        = (unsigned int)(BitsInUnignedInt / 8);
    unsigned int HighBits         = (unsigned int)(0xFFFFFFFF) << (BitsInUnignedInt - OneEighth);
    unsigned int hash             = 0;
    unsigned int test             = 0;

    while (*str)
    {
        hash = (hash << OneEighth) + (*str++);
        if ((test = hash & HighBits) != 0)
        {
            hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits));
        }
    }

    return (hash & 0x7FFFFFFF);
}


unsigned int APHash(const char *str)
{
    unsigned int hash = 0;
    int i;
 
    for (i=0; *str; i++)
    {
        if ((i & 1) == 0)
        {
            hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
        }
        else
        {
            hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
        }
    }
 
    return (hash & 0x7FFFFFFF);
}

unsigned int DJBHash(const char *str)
{
    unsigned int hash = 5381;
 
    while (*str)
    {
        hash += (hash << 5) + (*str++);
    }
 
    return (hash & 0x7FFFFFFF);
}

unsigned int BKDRHash(const char *str)
{
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;
 
    while (*str)
    {
        hash = hash * seed + (*str++);
    }
 
    return (hash & 0x7FFFFFFF);
}


unsigned int ELFHash(const char *str)
{
    unsigned int hash = 0;
    unsigned int x    = 0;
 
    while (*str)
    {
        hash = (hash << 4) + (*str++);
        if ((x = hash & 0xF0000000L) != 0)
        {
            hash ^= (x >> 24);
            hash &= ~x;
        }
    }
 
    return (hash & 0x7FFFFFFF);
}



int Bloomfilter::hashtable_init()
{
	hashtable.push_back(*PJWHash);
	hashtable.push_back(*JSHash);
	hashtable.push_back(*RSHash);
	hashtable.push_back(*SDBMHash);
	hashtable.push_back(*APHash);
	hashtable.push_back(*DJBHash);
	hashtable.push_back(*BKDRHash);
	hashtable.push_back(*ELFHash);
	return hashtable.size();

}



void Bloomfilter::bloomfilterinit(double err_rate,int num,char* path)
{
	mypath=path;
	samplenum=num;
	bitpoollen=-((samplenum*log(err_rate))/(log(2)*log(2)));
	hashfuncnum=0.7*(bitpoollen/samplenum);
	len=bitpoollen/32+1;
	bitpool=new int[len];
	filter_init();
}
int Bloomfilter::hashnum()
{

	return hashfuncnum;
}
int Bloomfilter::sizeofpool()
{

	return len;
}

void Bloomfilter::filter_init()
{
	hashtable_init();
	if(hashfuncnum>hashtable.size())
	{
		cout<<"哈系表中的函数不足,请添加"<<endl;
		return ;
	}
	listinit();				
}

bool Bloomfilter::is_contain(const char* str)
{
	int  hashval;
	for(int i=0;i!=hashfuncnum;i++)
	{
		hashval=hashtable[i](str);
		//cout<<hashval<<" ";   //test
		hashval=hashval%(len*32); //len*32为bitpool的总位数
		if(bitpool[hashval/32]&(0x1<<(hashval%32)))
			continue;
		else
			return false;

	}
	return true;
} 
void Bloomfilter::listinit()
{
        FILE* fp;
        char* buf;
        size_t length=0;
	fp=fopen(mypath,"r+");
	int hashval;
	char* p;
	while(getline(&buf,&length,fp)!=EOF)
	{
		
		p=buf;
		while(*p!='\n')
		{
			p++;
		}
		*p='\0';

		for(int i=0;i!=hashfuncnum;i++)
		{
			
			hashval=hashtable[i](buf);		
		//	cout<<hashval<<" ";	//test
			hashval=hashval%(len*32);
			bitpool[hashval/32]|=(0x1<<(hashval%32));
		}
	}
	fclose(fp);
}

Bloomfilter::~Bloomfilter()
{
	delete  []bitpool;
}

Bloomfilter*  Bloomfilter::pbloom=NULL;
Bloomfilter*  Bloomfilter::instance()
{
        if(pbloom==NULL)
                pbloom=new Bloomfilter();
        return pbloom;

}

