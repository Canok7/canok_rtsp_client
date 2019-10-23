#include "cH264RTP.h"
#include <stdlib.h>
#include <string.h>

#include"memwatch.h"

cH264RTP::cH264RTP(CQueue *QueNet,CQueue *QueH264):
cRTPParse(QueNet,QueH264),
mFUBufferOffset(0),
mFUBuffer(NULL)
{
	mFUBuffer = (u_int8_t *)malloc(512*1024);
}

cH264RTP::~cH264RTP()
{
	if(mFUBuffer)
	{
		free(mFUBuffer);
		mFUBuffer = NULL;
	}

}

NAL_TYPE cH264RTP::get_nal_type(uint8_t*data)
{
	char nal_type = LEFT_CHAR_BIT_GET(data,3,5);
	if(1 <=  nal_type && nal_type <= 23)
	{//single nal
		return SINGLE;
	}
	else if(nal_type == 28)
	{
		return FU_A;
	}
	else if(nal_type == 29)
	{
		return FU_B;
	}
	else 
	{
		DEBUG_WARN("----nal_type %d \n",nal_type);
		return UNDEFINED;
	}
	
	
}

int cH264RTP::parse_FU_A(u_int8_t *load_data, int dataLen)
{
	//fu indicator
	char fu_indicator = *(load_data);
	char fu_header = *(load_data+1);
	char nal =  (fu_indicator & 0xe0) | (fu_header & 0x1f);
	int h264Datalen = dataLen -2;
	u_int8_t *pH264Data = load_data+2;
	if(fu_header & 0x80)
	{//start
		DEBUG_INFO2("-- FU_A----start-------------\n");
		if(mFUBufferOffset !=0 )
		{
			//输出到h264队列，清空mFUBuffer
			pQueH264->push(mFUBuffer, mFUBufferOffset);
			mFUBufferOffset =0;
		}
	
		memcpy(mFUBuffer+mFUBufferOffset,&nal,1);
		mFUBufferOffset++;
		
		memcpy(mFUBuffer+mFUBufferOffset,pH264Data,h264Datalen);
		mFUBufferOffset += h264Datalen;
	}
	else if(fu_header & 0x40)
	{//end
	
		DEBUG_INFO2("-- FU_A------END------------\n");
		memcpy(mFUBuffer+mFUBufferOffset,pH264Data,h264Datalen);
		mFUBufferOffset += h264Datalen;

		//输出到h264队列，清空mFUBuffer
		pQueH264->push(mFUBuffer, mFUBufferOffset);
		mFUBufferOffset =0;
	}
	else 
	{
		DEBUG_INFO2("-- FU_A ------------\n");
		memcpy(mFUBuffer+mFUBufferOffset,pH264Data,h264Datalen);
		mFUBufferOffset += h264Datalen;
	}
}

int cH264RTP::parse_payload(uint8_t *pData, int dataLen)
{
	NAL_TYPE type = get_nal_type(pData);
	if(type == SINGLE)
	{
		DEBUG_INFO2("SINGLE FRAME ******** dataLen :%d\n",dataLen);
		pQueH264->push(pData, dataLen);
	}
	else if(type == FU_A)
	{//fu-a 分片，在这里解析
		parse_FU_A(pData, dataLen);
	}
	else 
	{
		DEBUG_ERR("not support for now type:%d\n",type);
	}
}

