#ifndef __header_cH264RTP__
#define __header_cH264RTP__

#include "cRTPParse.h"


enum NAL_TYPE{
	FU_A,
	FU_B,
	SINGLE,
	UNDEFINED,
};

class cH264RTP  :public cRTPParse
{
public :
	cH264RTP(CQueue *QueNet,CQueue *QueH264);
	~cH264RTP();

	
	virtual int parse_payload(uint8_t *pData,int dataLen);
private:
	NAL_TYPE get_nal_type(uint8_t*data);
	int parse_FU_A(u_int8_t *load_data, int dataLen);

private:
	u_int8_t *mFUBuffer;
	int mFUBufferOffset;
};
#endif
