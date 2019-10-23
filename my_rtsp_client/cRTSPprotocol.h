#ifndef __header_cRTSPprotocol__
#define __header_cRTSPprotocol__

#include<stdio.h>
#include<stdlib.h>
#include "com_net.h"


typedef bool Boolean;
#define False false
#define True true
typedef u_int16_t portNumBits;

/*
* 纯粹协议内容相关，rtsp 协议创建，解析
*/
typedef struct _media_session_
{
#if 0 //dont care
  char* fConnectionEndpointName;
  double fMaxPlayStartTime;
  double fMaxPlayEndTime;
  char* fAbsStartTime;
  char* fAbsEndTime;
//  struct in_addr fSourceFilterAddr; // used for SSM
  float fScale; // set from a RTSP "Scale:" header
  float fSpeed;
  char* fMediaSessionType; // holds a=type value
  char* fSessionName; // holds s=<session name> value
  char* fSessionDescription; // holds i=<session description> value
#endif
  char* fControlPath; //必须  ， 在SETUP   请求中需要，指定setup哪一个media       holds     optional a=control: string
  bool fMultiplexRTCPWithRTP;//是否 rtp rtcp复用一个socket，一般不服用，服务端定
}MEDIA_SESSION;

typedef struct _describe_info_
{
	MEDIA_SESSION media[3];//一个音频，一个视频，还一个保留
}DESCRIBE_INFO;


typedef struct _option_info_
{
	
}OPTON_INFO;

typedef struct _setup_info_
{
  unsigned short serverPortNum; //使用rtp-over-udp时需要，用于rtp创建udp socket
  	unsigned char rtpChannelId;//使用rtp-over-tcp时候需要
  	unsigned  rtcpChannelId; 
  char fSessionId[64];  //必须，在PLAY 请求中需要
}SETUP_INFO;

typedef struct _play_info_
{
}PLAY_INFO;

typedef struct _reply_info_
{
	int respondCode;
	#if 0
	OPTON_INFO*    pOption;
	DESCRIBE_INFO* pScribe;
	SETUP_INFO*    pSetup;
	PLAY_INFO* 	   pPlay;
	#else
	OPTON_INFO   pOption;
	DESCRIBE_INFO pScribe;
	SETUP_INFO   pSetup;
	PLAY_INFO   pPlay;
	#endif
}REPLY_INFO;

enum REQUEST_TYPE {
	REQUEST_OPTION,
	REQUEST_DESCRIBE,
	REQUEST_SETUP,
	REQUEST_PLAY,
	REQUEST_PAUSE,
	REQUEST_TEARDOWN,
};
class cRTSPprotocol
{
	public:
		cRTSPprotocol(int len);
		~cRTSPprotocol();
		int begain_cmd();
		int add_line_cmd(const       char*buf, int buflen);
		int end_cmd();

		
		int parse(char*buf,int buflen,REQUEST_TYPE request,unsigned cseq,int *extData);
		
		bool checkForHeader(char const* line, char const* headerName, unsigned headerNameLength, char const** headerParams);
		int parseResponseCode(char const* line, unsigned *responseCode);
		bool catchVideoMedinSession(char const*sdpMLine);
		bool parseSDPAttribute_control(char const* sdpLine);
		bool parseSDPAttribute_rtcpmux(char const* sdpLine);
		int parseSDP(char const* sdpDescription);
		bool parseTransportParams(char const* paramsStr,
					 char*& serverAddressStr, portNumBits& serverPortNum,
					 unsigned char& rtpChannelId, unsigned char& rtcpChannelId);
	private:
		bool parseSDPLine(char const* inputLine,
					   char const*& nextLine);
	public:
		char*mSendbuf;
		int mSendLen;
		int mSendOffset;

		REPLY_INFO pREPLY;
	
		
};
#endif
