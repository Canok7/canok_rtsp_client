#include "cRTPClient.h"
#include "com_net.h"
#include "com_debug.h"
#include <pthread.h>
#include <string.h>

#include "Pcapfile.h"

#include <unistd.h>

#ifdef MEM_WATCH
#include"memwatch.h"
#endif


cRTPClient::cRTPClient():
mbRead(false),
mbParse(false),
mfistSequenceNumber(0),
mlostCount(0),
serIp(NULL),
m_socket(-1),
m_port(0),
pQueNet(NULL),
pQueH264(NULL),
pRTPParse(NULL)
{
	memset(&mlastRtpInfo,0,sizeof(mlastRtpInfo));
}
cRTPClient::~cRTPClient()
{

}
int cRTPClient::rtp_init(const char* serip)
{
    serIp=strdup(serip);
//创建网络数据缓冲区，h264输出缓冲区，创建socket , 创建rtp读线程和rtp解析线程
	pQueNet = new CQueue(300,1500);
    pQueH264 = new CQueue(50,512*1024);
	

	m_socket = udp_net_setup();
    unsigned recvBUf = increaseReceiveBufferTo(m_socket,2000000);
    DEBUG_INFO1("rtp_UDP_recv_buf %u \n",recvBUf);
	return udp_get_port(m_socket, &m_port);

	#if 0
	pthread_t pth_read;
	if(pthread_create(&pth_read,NULL,&cRTPClient::read_thread,this))
	{
		DEBUG_ERR("read thread create err!\n");
	}

	pthread_t pth_parse;
	if(pthread_create(&pth_read,NULL,&cRTPClient::parse_thread,this))
	{
		DEBUG_ERR("parse thread create err!\n");
	}
	#endif

}
int cRTPClient::rtp_deinit()
{
//	先结束两个线程
	rtp_stop();
	show_stream_info();
	delete pRTPParse;

	closeSocket(m_socket);
	if(serIp)
	{
		free(serIp);
		serIp = NULL;
	}


	delete pQueNet;
	delete pQueH264;
	return RET_SUCESS;
}

int cRTPClient::rtp_start()
{
	int ret = RET_SUCESS;
	mbParse = true;
	mbRead = true;
	//开始线程
	if(pthread_create(&mpth_read,NULL,&cRTPClient::read_thread,this))
	{
		ret = RET_ERR;
		DEBUG_ERR("read thread create err!\n");
	}

	if(pthread_create(&mpth_parse,NULL,&cRTPClient::parse_thread,this))
	{
		ret = RET_ERR;
		DEBUG_ERR("parse thread create err!\n");
	}

	return ret;
}
int cRTPClient::rtp_stop()
{
	mbParse = false;
	mbRead = false;

	pthread_join(mpth_read,NULL);
	pthread_join(mpth_parse,NULL);
	return RET_SUCESS;
}
int cRTPClient::rtp_getUDPPort()

{
	return m_port;
}
int cRTPClient::rtp_UDPConnect(unsigned serPort)
{
	return udp_connect(m_socket, serIp, serPort);
}

int cRTPClient::rtp_requestFrameBuffer(unsigned char **pdata,int *plen)
{
	return pQueH264->getbuffer(pdata, plen);
}
int cRTPClient::rtp_releaseBuffer(int index)
{
	return pQueH264->releasebuffer(index);
}
int cRTPClient::rtp_getFrame(unsigned char *data, int len)
{
	return pQueH264->pop(data,len);
}
int cRTPClient::set_header_info(uint8_t* pHeader,RTP_PACK_HEADER_INFO* pHeaderInfo )
{
	if(pHeader == NULL || pHeaderInfo == NULL)
	{
		DEBUG_WARN("pram erro \n");
		return RET_ERR;
	}

	#if 0
	pHeaderInfo->version = pHeader[0] & (0x03);
	pHeaderInfo->marker = pHeader[1] & (0x01);
	pHeaderInfo->payloadType = pHeader[1] & ~(0x01);
	#endif
	pHeaderInfo->version = LEFT_CHAR_BIT_GET(pHeader,0,2);
	pHeaderInfo->marker = LEFT_CHAR_BIT_GET(pHeader+1,0,1);
	pHeaderInfo->payloadType = LEFT_CHAR_BIT_GET(pHeader+1,1,7);

	
	pHeaderInfo->sequenceNumber = LITTLE_MAKE_UNINT16(pHeader+2);
	pHeaderInfo->timeStamp = LITTLE_MAKE_UNINT32(pHeader+4);
	pHeaderInfo->ssrc = LITTLE_MAKE_UNINT32(pHeader+8); 
	DEBUG_INFO1("sequenceNumber %u timeStamp %u ssrc %u payloadType %d \n",pHeaderInfo->sequenceNumber,pHeaderInfo->timeStamp,pHeaderInfo->ssrc,pHeaderInfo->payloadType );
	return RET_SUCESS;
}
void cRTPClient::show_stream_info()
{
	DEBUG_WARN("all: rtp frame had losted  %d, expect:%d \n",mlostCount>0?mlostCount:0,mlastRtpInfo.sequenceNumber-mfistSequenceNumber);
}

bool cRTPClient::check_header(RTP_PACK_HEADER_INFO* pHeaderInfo)
{
	if(mfistSequenceNumber == 0)
	{// the fist frame
		mfistSequenceNumber = pHeaderInfo->sequenceNumber;
		memcpy(&mlastRtpInfo, pHeaderInfo,sizeof(mlastRtpInfo));

		if(pHeaderInfo->payloadType == 96 && pRTPParse == NULL)
		{
		    DEBUG_INFO3("carete h264rtpParser \n");
			pRTPParse = new cH264RTP(pQueNet,pQueH264);
		}
		return true;
	}
	if(pHeaderInfo->ssrc == mlastRtpInfo.ssrc)
	{
		if(pHeaderInfo->payloadType == mlastRtpInfo.payloadType)
		{
			if(pHeaderInfo->sequenceNumber - mlastRtpInfo.sequenceNumber == 1)
			{
			}
			else
			{
				mlostCount += pHeaderInfo->sequenceNumber - 1 - mlastRtpInfo.sequenceNumber;
				DEBUG_WARN("all: rtp frame had losted mlostCount %d ,expect:%d fistseq:%u curseq: %u lastseq: %u\n",mlostCount,pHeaderInfo->sequenceNumber-mfistSequenceNumber,mfistSequenceNumber,pHeaderInfo->sequenceNumber,mlastRtpInfo.sequenceNumber);
			    if(pHeaderInfo->sequenceNumber < mlastRtpInfo.sequenceNumber)
                {
                    DEBUG_WARN("seq erro lastseq: %u curseq: %u \n",mlastRtpInfo.sequenceNumber,pHeaderInfo->sequenceNumber);
                }
			}
			memcpy(&mlastRtpInfo, pHeaderInfo,sizeof(mlastRtpInfo));
			return true;
		}
		else
		{
			DEBUG_INFO3("loadtype %d  lastloadtype %d\n",pHeaderInfo->payloadType,mlastRtpInfo.payloadType);
		}
	}
	else
	{	
		DEBUG_INFO3("SSRC erro pHeaderInfo->ssrc %u mlastRtpInfo.ssrc %u \n",pHeaderInfo->ssrc, mlastRtpInfo.ssrc);
	}
	
	return false;
}

int cRTPClient::parse()
{
	uint8_t *pData;
	int dataLen=0,index = -1;
	RTP_PACK_HEADER_INFO rtpInfo;
	index = pQueNet->getbuffer(&pData, &dataLen);
	if(index >= 0)
	{
		set_header_info(pData,&rtpInfo);
		if(check_header(&rtpInfo))
		{
		    if(pRTPParse)
		    {
                pRTPParse->parse_payload(pData + 12, dataLen - 12);
            } else{
		        DEBUG_INFO3("no parser\n");
		    }
		}
		else
		{
			DEBUG_ERR("head check erro\n");
		}
		pQueNet->releasebuffer(index);
	}

	return RET_SUCESS;
}
void* cRTPClient::read_thread(void *pRtpclient)
{
	cRTPClient * pRtp = (cRTPClient *)pRtpclient;

	
	DEBUG_INFO1("fread ------------\n\n");
#if 0
	FILE *fp_rtp_data = fopen("./fp_rtp_data","w+");
	char my_head[64]={0};
	
	if(fp_rtp_data == NULL)
	{
		DEBUG_ERR("fopen erro \n");
	}
	setvbuf(fp_rtp_data,NULL,_IONBF,0);
	char *buffer = (char *)malloc(2000);
	int ret =0;
	while(1)
	{
		ret = readSocket(pRtp->m_socket, buffer, 2000);
		if(ret > 0 && fp_rtp_data)
		{
			snprintf(my_head,sizeof(my_head),"udp_data:%d",ret);
			fwrite(my_head,1,sizeof(my_head),fp_rtp_data);
			fwrite(buffer,1,ret,fp_rtp_data);
		}
	}
	fclose(fp_rtp_data);
#else
	#if 1
		int ret =0;
		uint8_t *buffer = (uint8_t *)malloc(1500);
		
		while(pRtp->mbRead)
		{
			ret = readSocket(pRtp->m_socket, (char *)buffer, 2000);
			if(ret >0)
			{
				pRtp->pQueNet->push(buffer, ret);
			}
		}
		free(buffer);
	#elif 0
		int ret =0;
		uint8_t buffer[1024*20]={0};
		CpcapPare mpcap;
		mpcap.init("my_rtsp.pcap",0);
		while((ret = mpcap.getFrames(buffer,sizeof(buffer)))>0)
		{
			pRtp->pQueNet->push(buffer, ret);
		}
	#else
		int ret =0,datalen = 0;
		uint8_t head[64]={0};
		uint8_t *buffer = (uint8_t*)malloc(1024*30);
		FILE *fp_test = fopen("fp_rtp_data","r");
		if(fp_test == NULL)
		{
			DEBUG_ERR("erro to open file \n");
		}

		while(1)
		{
			memset(head,0,sizeof(head));
			ret = fread(head,1,sizeof(head),fp_test);
			//DEBUG_INFO1("fread %s\n",head);
			usleep(1000000/50);
			if(ret == sizeof(head) )
			{
				if(sscanf((const char*)head, "udp_data:%d", &datalen) != 1)
				{
					DEBUG_ERR("scan erro:head:%s\n",head);
					break;
				}
				
				ret = fread(buffer,1,datalen,fp_test);
				if(ret < datalen)
				{
					DEBUG_INFO1("read erro: ret %d\n",ret);
					break;
				}
				else
				{
					pRtp->pQueNet->push(buffer, ret);
				}
				
			}
			else
			{
				DEBUG_INFO1("read erro: ret %d\n",ret);
				break;
			}
		}

		fclose(fp_test);
		free(buffer);
		
	#endif
#endif
		return NULL;
}
void* cRTPClient::parse_thread(void *pRtpclient)
{
	cRTPClient * pRtp = (cRTPClient *)pRtpclient;
	while(pRtp->mbParse)
	{
		pRtp->parse();
	}
	return NULL;
}



