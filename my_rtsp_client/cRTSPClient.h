#ifndef __header_cRTSPClient__
#define __header_cRTSPClient__

#include <stdio.h>
#include "cRTPClient.h"
#include "cRTSPprotocol.h"
#define RTSP_BUFFER_LEN (1024*2)
class cRTSPClient 
{
	public:
		cRTSPClient();
		~cRTSPClient();
		int api_init(const char*url);
		int api_deinit();
		int api_requestFrameBuffer(unsigned char **pdata,int *plen);
		int api_releaseBuffer(int index);
		int api_getFrame(unsigned char *data, int len);

		/*
		*    发送完请求后的默认处理，根据服务端的回复做相应的处理
		*/
		virtual int after_options();
		virtual int after_describe();
		virtual int after_setup();
		virtual int after_play();
		virtual int after_pause();
		virtual int after_teardown();
	private:
		int connect();
		
		/*
		*	发送rtsp请求命令，并读取服务端的回复
		*/
		int rtsp_request(char* buffer, unsigned bufferSize,REQUEST_TYPE request);
		
		int request_options();
		int request_describe();
		int request_setup();
		int request_play();
		int request_pause();
		int request_teardown();

		int parse_rtsp_url(const char* url,char*ip,unsigned *port );
	private:
		char mUrl[128];
		char serverIp[16];
		unsigned serverPort;
		int m_cseq;
		int m_socket;
		char* m_rtsp_buffer;
		FILE*fp_prot_log;
		cRTSPprotocol pr;
		cRTPClient *pRTP;
		
};
#endif
