#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cRTSPClient.h"
#include"memwatch.h"

int main(int argc, const char *argv[])
{
#ifdef EN_MEMWATCH
	mwInit();
#endif

	cRTSPClient *client = new cRTSPClient();
	unsigned char *pdata = NULL;
	int dataLen =0;
	char nal[4]={0x00,0x00,0x00,0x01};
	FILE *fp_out = fopen("./live_out","w+");
	if(fp_out == NULL)
	{
		printf("erro to open file\n");
		return -1;
	}
	if(argc != 2)
	{	char defUrl[]="rtsp://127.0.0.1:8989/stream";
		printf("usage:url\n");
		printf("use defalut url:%s\n",defUrl);
		
		client->api_init(defUrl);
	}
	else
	{
		client->api_init(argv[1]);
	}
	
	int count = 300;
	while(count--)
	{
		int index =client->api_requestFrameBuffer(&pdata, &dataLen);
		if(index >= 0)
		{
			fwrite(nal,1,sizeof(nal),fp_out);
			fwrite(pdata,1,dataLen,fp_out);
			client->api_releaseBuffer(index);
		}
	}
	fclose(fp_out);
	client->api_deinit();
	delete client;

	printf("wang over-----\n\n");
	
#ifdef EN_MEMWATCH
	mwTerm();
#endif
}
