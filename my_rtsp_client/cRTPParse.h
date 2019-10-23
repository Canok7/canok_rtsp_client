#ifndef __header_cRTPParse__
#define __header_cRTPParse__
#include "com_queue.h"
#include "com_debug.h"


class cRTPParse
{
public:
	cRTPParse(CQueue *QueNet,CQueue *QueH264);
	~cRTPParse();

	virtual int parse_payload(uint8_t *pData,int dataLen)=0;

//	from rtpClient 
public:
	CQueue *pQueNet;
	CQueue *pQueH264;
};

#endif


