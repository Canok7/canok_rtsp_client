#ifndef  __PCAPFILE_HEAD_H__
#define __PCAPFILE_HEAD_H__

typedef unsigned char	uint8_t;	 //无符号8位数

class CpcapPare
{
	public:
		CpcapPare();
		~CpcapPare();
		int init(const char *infile, int bReloop);
		int release();
		int getFrames(uint8_t *buf,int len);
		
	private:
		int mReloop;
		FILE *fpin;
		
};
#endif
