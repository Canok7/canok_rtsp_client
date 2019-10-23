#include<stdio.h>
#include <unistd.h>
#include "Pcapfile.h"

#if 0
#define TRACK(x...)  printf("wang %d %s",__LINE__,__FUNCTION__);printf(x)
#else
#define TRACK(x...)   

#endif
CpcapPare::CpcapPare():
mReloop(0),
fpin(NULL)
{

}

CpcapPare::~CpcapPare()
{

}

static unsigned long getLittleData(int len, uint8_t*buf)
{
	if(len <=0)
	{
		return 0;
	}
	unsigned long ret = 0;
	while(len--)	
	{	
		ret = ret <<8;
		ret |= buf[len];
	}
	return ret;
}

int CpcapPare::init(const char *infile, int bReloop)
{
	fpin = fopen(infile,"rb");
	if(fpin == NULL)
	{
		printf("erro to open file\n");
		return -1;
	}
	fseek(fpin,24,SEEK_SET);
	mReloop = bReloop;
	return 0;
}

int CpcapPare::release()
{
	if(fpin != NULL)
	{
		fclose(fpin);
	}
	return 0;
}


int CpcapPare::getFrames(uint8_t *buf,int len)
{

uint8_t packethead[16]={0};
int datalen = 0;
testfread:

	int ret =fread(packethead,1,16,fpin);
	if(ret < 16)
	{
		if(mReloop)
		{
			fseek(fpin,24,SEEK_SET);
			TRACK("reloop! ret %d \n",ret);
			goto testfread;
		}
		else
		{
			return -1;
		}
	}
	datalen =  getLittleData(4,packethead+8);
	
	TRACK("  %d: %d\n",packethead[8],packethead[9]);
	TRACK("datalen %d ret %d\n",datalen,ret);
	
	ret = fread(buf,1,datalen,fpin);
	if(ret < datalen)
	{
		if(mReloop)
		{
			//循环读取文件
			fseek(fpin,24,SEEK_SET);
			TRACK("reloop ret %d !\n",ret);
			goto testfread;
		}
		else
		{
			return -1;
		}
		
	}
	return ret;
}
		

#if 0
int main(int argc, const char *argv[])
{
	if(argc != 3)
	{
		printf("use:inputfile outfile\n");
		return -1;
	}
	CpcapPare mpcap;
	mpcap.init(argv[1],0);
	char buffer[1024*20]={0};
	FILE *fpout = fopen(argv[2],"w+");
	int ret =0;
	while((ret = mpcap.getFrames(buffer,sizeof(buffer)))>0)
	{
		fwrite(buffer,1,ret,fpout);
	}
	fclose(fpout);
	
}
#endif

