#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "cRTSPprotocol.h"
#include "cRTSPClient.h"
#include "com_net.h"
#include "com_debug.h"

#ifdef MEM_WATCH
#include"memwatch.h"
#endif

cRTSPClient::cRTSPClient():
m_cseq(1),
pr(256),//
m_socket(-1),
fp_prot_log(NULL),
m_rtsp_buffer(NULL),
pRTP(NULL)
{
	memset(mUrl,0,sizeof(mUrl));
}
cRTSPClient::~cRTSPClient()
{
}

int cRTSPClient::api_init(const char*url)
{
	if(NULL == fp_prot_log)
	{
		fp_prot_log = fopen("rtsp_log","w+");
		if(fp_prot_log == NULL)
		{
			DEBUG_ERR("rtsp_log open failed!");
			return RET_ERR;
		}
		setbuf(fp_prot_log,NULL);
	}
	if(NULL == m_rtsp_buffer)
	{
		m_rtsp_buffer = (char*)malloc(RTSP_BUFFER_LEN);
		if(NULL == m_rtsp_buffer)
		{
			DEBUG_ERR("buffer malloc erro\n");
		}
	}

	if(parse_rtsp_url(url,serverIp,&serverPort) == RET_SUCESS)
	{
		if(connect() == RET_SUCESS)
		{
			if(request_options() == RET_SUCESS)
			{
				if(request_describe() == RET_SUCESS)
				{
					if(request_setup() == RET_SUCESS)
					{
						if(request_play() == RET_SUCESS)
						{
							return RET_SUCESS;
						}
					}
				}
			}
		}
	}

	return RET_ERR;
}
int cRTSPClient::api_deinit()
{
	close(m_socket);
	fclose(fp_prot_log);
	fp_prot_log = NULL;

	free(m_rtsp_buffer);
	m_rtsp_buffer = NULL;

	if(pRTP)
	{
		pRTP->rtp_deinit();
		delete pRTP;
		pRTP = NULL;
	}

	return RET_SUCESS;
}
int cRTSPClient::api_requestFrameBuffer(unsigned char **pdata,int *plen)
{
	if(pRTP)
	{
		return pRTP->rtp_requestFrameBuffer(pdata, plen);
	}
	else
	{
		return RET_ERR;
	}

}
int cRTSPClient::api_releaseBuffer(int index)
{
	if(pRTP)
	{
		return pRTP->rtp_releaseBuffer(index);
	}
	else
	{
		return RET_ERR;
	}

}
int cRTSPClient::api_getFrame(unsigned char *data, int len)
{
	if(pRTP)
	{
		return pRTP->rtp_getFrame(data, len);
	}
	else
	{
		return RET_ERR;
	}
}
/*
*	发送rtsp请求命令，并读取服务端的回复
*/
int cRTSPClient::rtsp_request(char* buffer, unsigned bufferSize,REQUEST_TYPE request)
{
	int ret = RET_ERR;

	if(fp_prot_log)
	{
		fwrite("c->s:::::::\n",1,12,fp_prot_log);
		fwrite(buffer,1,bufferSize,fp_prot_log);
	}
	writeSocket(m_socket, buffer, bufferSize);

	//需要在这里读，等待服务器的回应
	#if 0 
	//干脆不处理，假装已经成功了
	sleep(1);
	#else
	ret = makeSocketBlocking(m_socket, 50);// 50 ms
	int dataAll=0,count =5,dataRead=0;
	//确保一次性读完，而不用在parse里面再读,所以这里干脆 no 多读几次
	memset(m_rtsp_buffer,0,RTSP_BUFFER_LEN);
	while(count--)
	{	
		if( (dataRead = readSocket(m_socket,m_rtsp_buffer+dataAll,RTSP_BUFFER_LEN-dataAll) )!= -1) 
		{
			dataAll += dataRead;
		}
		
		DEBUG_INFO3("read count %d ,dataRead %d dataAll %d \n",count,dataRead,dataAll);
	}
	
	if(fp_prot_log)
	{
		fwrite("s->c===========\n",1,12,fp_prot_log);
		fwrite(m_rtsp_buffer,1,dataAll,fp_prot_log);
		//fflush(fp_prot_log);
	}
	
	ret = pr.parse(m_rtsp_buffer, dataAll, request,m_cseq,NULL);
	m_cseq ++;

	return ret;
	#endif
}

int cRTSPClient::connect()
{
	m_socket = tcp_net_conncet(serverIp,serverPort);
	return m_socket==-1?RET_ERR:RET_SUCESS;
}
int cRTSPClient::request_options()
{
#if 0
	//标准的协议以 \r\n作为换行符，整个消息以一个空行作为结尾，
	char buf[]={"OPTIONS rtsp://127.0.0.1:8989/stream RTSP/1.0 \r\nCSeq:1\r\n\r\n"};
	//这里 sizeof(buf)-1, 字符串最后有一个 “\0” ，剔除
	rtsp_request(buf,sizeof(buf)-1,REQUEST_OPTION); 
#else
	char buf[256]={0};
	pr.begain_cmd();do{
		snprintf(buf,sizeof(buf),"OPTIONS %s %s",mUrl,"RTSP/1.0");
		pr.add_line_cmd(buf, strlen(buf));
		snprintf(buf,sizeof(buf),"CSeq:%d",m_cseq);
		pr.add_line_cmd(buf, strlen(buf));
	}while(0);pr.end_cmd();
	rtsp_request(pr.mSendbuf,pr.mSendOffset,REQUEST_OPTION);
#endif	
	after_options();
	return RET_SUCESS;
}
int cRTSPClient::request_describe()
{
	 int ret = RET_SUCESS;
#if 0
	char buf[]={"DESCRIBE rtsp://127.0.0.1:8989/stream RTSP/1.0 \r\nCSeq:2\r\n\r\n"};
	rtsp_request(buf,sizeof(buf)-1,REQUEST_DESCRIBE);
#else
	char buf[256];
	pr.begain_cmd();do{
		snprintf(buf,sizeof(buf),"DESCRIBE %s %s",mUrl,"RTSP/1.0");
		pr.add_line_cmd(buf, strlen(buf));
		snprintf(buf,sizeof(buf),"CSeq:%d",m_cseq);
		pr.add_line_cmd(buf, strlen(buf));
	}while(0);pr.end_cmd();
	if(RET_SUCESS == rtsp_request(pr.mSendbuf,pr.mSendOffset,REQUEST_DESCRIBE))
	{
		ret = after_describe();
	}
	else
	{
		ret = RET_ERR;
	}
#endif
	return ret;
}
int cRTSPClient::request_setup()
{
  int ret= RET_SUCESS;
#if 0
// trackID=0 是从服务器的响应中获取到的
//使用udp 传输 Transport:RTP/AVP;unicast;client_port=50400-50401\r\n
//已经把使用udp 端口， 50400  50401 .. 两个track，两个端口
// 在setup之前或者之后，创建接受端的rtp socket


//在这里获取cli端创建的udp 的port, 给到服务器。
	char buf[]={"SETUP rtsp://127.0.0.1:8989/stream/trackID=0 RTSP/1.0 \r\nCSeq:3\r\nTransport:RTP/AVP;unicast;client_port=50400-50401\r\n\r\n"};
	rtsp_request(buf,sizeof(buf)-1,REQUEST_SETUP);
#else
	char buf[256];
	pr.begain_cmd();do{
		//only  setup video
		snprintf(buf,sizeof(buf),"SETUP %s %s",pr.pREPLY.pScribe.media[0].fControlPath,"RTSP/1.0");
		pr.add_line_cmd(buf, strlen(buf));
		snprintf(buf,sizeof(buf),"CSeq:%d",m_cseq);
		pr.add_line_cmd(buf, strlen(buf));
		
		//添加rtp端口
		int port =pRTP->rtp_getUDPPort();
		snprintf(buf,sizeof(buf),"Transport:RTP/AVP;unicast;client_port=%d-%d",port,port+1);
		pr.add_line_cmd(buf,strlen(buf));
	}while(0);pr.end_cmd();

	if(RET_SUCESS == rtsp_request(pr.mSendbuf,pr.mSendOffset,REQUEST_SETUP))
	{
		ret = after_setup();
	}
	else
	{
		ret = RET_ERR;
	}
#endif
	return ret;
}
int cRTSPClient::request_play()
{
	int ret = RET_SUCESS;
#if 0
	char buf[]={"PLAY rtsp://127.0.0.1:8989/stream RTSP/1.0 \r\nCSeq:4\r\nRange:npt=0.000-\r\nSession:12345678\r\n\r\n"};
	rtsp_request(buf,sizeof(buf)-1,REQUEST_PLAY);
#else
	char buf[256];
	pr.begain_cmd();do{
		snprintf(buf,sizeof(buf),"PLAY %s %s",mUrl,"RTSP/1.0");
		pr.add_line_cmd(buf, strlen(buf));
		snprintf(buf,sizeof(buf),"CSeq:%d",m_cseq);
		pr.add_line_cmd(buf, strlen(buf));
		snprintf(buf,sizeof(buf),"nRange:npt=0.000-");
		pr.add_line_cmd(buf, strlen(buf));
		snprintf(buf,sizeof(buf),"Session:%s",pr.pREPLY.pSetup.fSessionId);
		pr.add_line_cmd(buf, strlen(buf));
	}while(0);pr.end_cmd();
	if(RET_SUCESS == rtsp_request(pr.mSendbuf,pr.mSendOffset,REQUEST_PLAY))
	{
		ret = after_play();
	}
	else
	{
		ret = RET_ERR;
	}
#endif
	return ret;
}
int cRTSPClient::request_pause()
{
	int ret = RET_SUCESS;
#if 0
	char buf[]={"PAUSE rtsp://127.0.0.1:8989/stream RTSP/1.0 \r\nCSeq:5\r\nSession:12345678\r\n\r\n"};
	rtsp_request(buf,sizeof(buf)-1,REQUEST_PAUSE);
	after_pause();
#else
	char buf[256];
	pr.begain_cmd();do{
		snprintf(buf,sizeof(buf),"PAUSE %s %s",mUrl,"RTSP/1.0");
		pr.add_line_cmd(buf, strlen(buf));
		snprintf(buf,sizeof(buf),"CSeq:%d",m_cseq);
		pr.add_line_cmd(buf, strlen(buf));
		snprintf(buf,sizeof(buf),"Session:%s",pr.pREPLY.pSetup.fSessionId);
		pr.add_line_cmd(buf, strlen(buf));
	}while(0);pr.end_cmd();
	if(RET_SUCESS == rtsp_request(pr.mSendbuf,pr.mSendOffset,REQUEST_PAUSE))
	{
		ret = after_pause();
	}
	else
	{
		ret = RET_ERR;
	}
#endif	
	return ret;
}
int cRTSPClient::request_teardown()
{
	int ret = RET_SUCESS;
#if 0
	char buf[]={"TEARDOWN rtsp://127.0.0.1:8989/stream RTSP/1.0 \r\nCSeq:5\r\nSession:12345678\r\n\r\n"};
	rtsp_request(buf,sizeof(buf)-1,REQUEST_TEARDOWN);
	after_teardown();
#else
	char buf[256];
	pr.begain_cmd();do{
		snprintf(buf,sizeof(buf),"TEARDOWN %s %s",mUrl,"RTSP/1.0");
		pr.add_line_cmd(buf, strlen(buf));
		snprintf(buf,sizeof(buf),"CSeq:%d",m_cseq);
		pr.add_line_cmd(buf, strlen(buf));
		snprintf(buf,sizeof(buf),"Session:%s",pr.pREPLY.pSetup.fSessionId);
		pr.add_line_cmd(buf, strlen(buf));
	}while(0);pr.end_cmd();
	if(RET_SUCESS == rtsp_request(pr.mSendbuf,pr.mSendOffset,REQUEST_TEARDOWN))
	{
		ret = after_teardown();
	}
	else
	{
		ret = RET_ERR;
	}
#endif	
	return ret;
}

int cRTSPClient::parse_rtsp_url(const char* url,char*ip,unsigned *port )
{
//eg :   rtsp://127.0.0.1:8989/stream
	DEBUG_INFO1("open [%s]\n",url);
	if(strlen(url)+1>sizeof(mUrl))
	{
		return RET_ERR;
	}
	strcpy(mUrl,url);

	char const* prefix = "rtsp://";
	unsigned const prefixLength = 7;
	if(strncasecmp(url, prefix, prefixLength) != 0)
	{
		DEBUG_ERR("URL format err\n");
		return RET_ERR;
	}
	char other[100]={0};
	if(sscanf(url+7,"%[^:]%*c%u%s",ip,port,other) != 3)
	{
		DEBUG_ERR("URL parse erro url:%s\n",url);
		return RET_ERR;
	}

	serverIp[sizeof(serverIp)-1]='\0';
	DEBUG_INFO3("URL PARSE ok:[%s] [%s %u] [other:%s]\n",mUrl,serverIp,serverPort,other);
	return RET_SUCESS;
}

int cRTSPClient::after_options()
{
	DEBUG_INFO1("options ok!\n");
	return RET_SUCESS;
}
int cRTSPClient::after_describe()
{
	int ret = RET_ERR;
	if(pr.pREPLY.respondCode == 200)
	{
		DEBUG_INFO1("describe ok!\n");
		//这里创建 rtp rtcp   ，服务器返回 setup 成功时候已经把 udp创建好了，
		pRTP = new cRTPClient();
		ret = pRTP->rtp_init(serverIp);
	}
	return ret;
}
int cRTSPClient::after_setup()
{
 	int ret = RET_ERR;
	if(pr.pREPLY.respondCode == 200)
	{
		DEBUG_INFO1("setup ok!\n");
		// 从返回信息中 得到server端的rtp的port，将cli的rtp socket connect上去
		ret =  pRTP->rtp_UDPConnect(pr.pREPLY.pSetup.serverPortNum);
		DEBUG_INFO3("udp connect ret %d \n",ret);
	}
	return ret;
}
int cRTSPClient::after_play()
{
	//开始读取数据
	DEBUG_INFO1("play ok!\n");
	return pRTP->rtp_start();
}
int cRTSPClient::after_pause()
{
	DEBUG_INFO1("pause ok!\n");
	return RET_SUCESS;
}
int cRTSPClient::after_teardown()
{
	DEBUG_INFO1("teardown ok!\n");
	return RET_SUCESS;
}


