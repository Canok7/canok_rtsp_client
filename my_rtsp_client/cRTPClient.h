#ifndef __header_cRTPClient__
#define __header_cRTPClient__

#include<stdio.h>
#include<stdlib.h>
#include "com_queue.h"
#include "cRTPParse.h"
#include "cH264RTP.h"

typedef struct _rtp_pack_header_
{
	char header[12];
	
}RTP_PACK_HEADER;

typedef struct _rtp_pack_header_info
{
	char version;
	char marker;
	char payloadType;
	u_int16_t sequenceNumber;
	u_int32_t timeStamp;
	u_int32_t ssrc;
}RTP_PACK_HEADER_INFO;

class cRTPClient
{
	public:
		cRTPClient();
		~cRTPClient();
		
		int rtp_init(const char* ser_host);
		int rtp_deinit();
		
		int rtp_start();
		//等到线程结束完，才返回
		int rtp_stop();
		int rtp_getUDPPort();
		int rtp_UDPConnect(unsigned serPort);
		int rtp_requestFrameBuffer(unsigned char **pdata,int *plen);
		int rtp_releaseBuffer(int index);
		int rtp_getFrame(unsigned char *data, int len);
	private:
		char *serIp;
		int16_t m_port;
		int m_socket;
		CQueue *pQueNet;
		CQueue *pQueH264;
		cRTPParse *pRTPParse;
		RTP_PACK_HEADER_INFO mlastRtpInfo;
	

		bool mbRead;
		bool mbParse;
		pthread_t mpth_read;
		pthread_t mpth_parse;

		u_int16_t mfistSequenceNumber;
		int mlostCount;
	private:
		static void *read_thread(void *);
		static void *parse_thread(void *);

		int parse();
		int set_header_info(uint8_t * pHeader,RTP_PACK_HEADER_INFO* pHeaderInfo );
		bool check_header(RTP_PACK_HEADER_INFO* pHeaderInfo);
		void show_stream_info();
		
};
#endif
