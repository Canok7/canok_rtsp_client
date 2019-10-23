#include"cRTSPprotocol.h"
#include"com_debug.h"
#include <string.h>

#ifdef MEM_WATCH
#include"memwatch.h"
#endif

cRTSPprotocol::cRTSPprotocol(int len):
mSendLen(len),
mSendbuf(NULL),
mSendOffset(0)
{
	mSendbuf = (char*)malloc(mSendLen);
	if(mSendbuf == NULL)
	{
		DEBUG_ERR("mallo erro\n");
	}
}
cRTSPprotocol::~cRTSPprotocol()
{
	free(mSendbuf);
	if(pREPLY.pScribe.media[0].fControlPath)
	{
		free(pREPLY.pScribe.media[0].fControlPath);
		pREPLY.pScribe.media[0].fControlPath = NULL;
	}
}

int cRTSPprotocol::begain_cmd()
{
	memset(mSendbuf,0,mSendLen);
	mSendOffset =0;
}

int cRTSPprotocol::add_line_cmd(const       char*buf, int buflen)
{
	if(mSendOffset + buflen + 2 > mSendLen)
	{
		DEBUG_ERR("add buf too long ,%d out of %d\n",buflen,mSendLen);
		return RET_ERR;
	}
	memcpy(mSendbuf+mSendOffset,buf,buflen);
	mSendbuf[mSendOffset+buflen] = '\r';
	mSendbuf[mSendOffset+buflen+1] = '\n';
	mSendOffset += buflen+2;
}

int cRTSPprotocol::end_cmd()
{
	mSendbuf[mSendOffset] = '\r';
	mSendbuf[mSendOffset+1] = '\n';
	mSendOffset += 2;
}

static char* getLine(char* startOfLine)
{//copy frome live555
  // returns the start of the next line, or NULL if none.  Note that this modifies the input string to add '\0' characters.
  for (char* ptr = startOfLine; *ptr != '\0'; ++ptr) 
  {
    // Check for the end of line: \r\n (but also accept \r or \n by itself):
    if (*ptr == '\r' || *ptr == '\n') 
	{
      // We found the end of the line
      if (*ptr == '\r') 
	  {
		*ptr++ = '\0'; //把 \r 换成了 0，
		if (*ptr == '\n') //跳过后面的 \n,返回下一个
			++ptr;
      } 
	  else 
	  {
        *ptr++ = '\0'; //把 \n 换成了 0，返回下一个
      }
      return ptr;
    }
  }

  return NULL;
}
//return blank lin start addr
static char*findBlankLine(char* buf,int buflen,bool *bEnd)
{
//getLine() will change the buf, so copy bak
    char* headerDataCopy = (char*)malloc(buflen);
	strncpy(headerDataCopy, buf, buflen);
	headerDataCopy[buflen-1]='\0';

	do{
		char *lineStart = headerDataCopy;
		char *nextLineStart = headerDataCopy;
		while(1)
		{
			nextLineStart = getLine(lineStart);
			//DEBUG_INFO3("beforline:%s\n",lineStart);
			if(lineStart == NULL) 
			{
				//DEBUG_INFO3("NO Blank line\n");
				free(headerDataCopy);
				*bEnd = true;
				return NULL;
			}
			if (lineStart[0] == '\0') 
			{
				//DEBUG_INFO3("lineStart:%s\n",lineStart);
				free(headerDataCopy);
				*bEnd = false;
				return buf+(lineStart-headerDataCopy);
			}
			
			lineStart = nextLineStart;
		}
	}while(0);
	free(headerDataCopy);

	*bEnd = true;
	return NULL;
}

bool cRTSPprotocol::checkForHeader(char const* line, char const* headerName, unsigned headerNameLength, char const** headerParams) 
{//copy frome live555
  if (strncasecmp(line, headerName, headerNameLength) != 0) return false;

  // The line begins with the desired header name.  Trim off any whitespace, and return the header parameters:
  unsigned paramIndex = headerNameLength;
  while (line[paramIndex] != '\0' && (line[paramIndex] == ' ' || line[paramIndex] == '\t')) ++paramIndex;
  if (line[paramIndex] == '\0') return true; // the header is assumed to be bad if it has no parameters

  *headerParams = &line[paramIndex];
  return true;
}


int cRTSPprotocol::parseResponseCode(char const* line, unsigned *responseCode) 
{
	unsigned code =0;
	if (sscanf(line, "RTSP/%*s%u", &code) != 1)
	{	
		return RET_ERR;
	}
	*responseCode  = code;
	return RET_SUCESS;
}

bool cRTSPprotocol::catchVideoMedinSession(char const*sdpMLine)
{
	//有一个媒体消息了，即对应一个rtp流，解析相关信息
	char mediumName[64] ={0};
	short unsigned  port =0; // 服务器端返回的 对cli要求的端口，一般为0 ，不限制，由客户端自己定
    unsigned payloadFormat;
	if((sscanf(sdpMLine, "m=%s %hu RTP/AVP %u",
				mediumName, &port, &payloadFormat) == 3 ||
			sscanf(sdpMLine, "m=%s %hu/%*u RTP/AVP %u",
				mediumName, &port, &payloadFormat) == 3)
			&& payloadFormat <= 127)
	{
			if(strncasecmp(mediumName, "video", 5) == 0)
			{//找到 video 媒体信息，ok! 
				return true;
			}
			else
			{//继续找 video信息，一般情况就两个 vido 和audio
				return false;
			}
	}
}

//获取一行
 bool cRTSPprotocol::parseSDPLine(char const* inputLine,
					   char const*& nextLine)
{
	  // Begin by finding the start of the next line (if any):
	  nextLine = NULL;
	  for (char const* ptr = inputLine; *ptr != '\0'; ++ptr) {
		if (*ptr == '\r' || *ptr == '\n') {
		  // We found the end of the line
		  ++ptr;
		  while (*ptr == '\r' || *ptr == '\n') ++ptr;
		  nextLine = ptr;
		  if (nextLine[0] == '\0') nextLine = NULL; // special case for end
		  break;
		}
	  }
	
	  // Then, check that this line is a SDP line of the form <char>=<etc>
	  // (However, we also accept blank lines in the input.)
	  if (inputLine[0] == '\r' || inputLine[0] == '\n') return true;
	  if (strlen(inputLine) < 2 || inputLine[1] != '='
		  || inputLine[0] < 'a' || inputLine[0] > 'z') {
		DEBUG_ERR("Invalid SDP line:%s \n", inputLine);
		return false;
	  }
	
	  return true;
}

bool cRTSPprotocol::parseSDPAttribute_rtcpmux(char const* sdpLine) 
{
  if (strncmp(sdpLine, "a=rtcp-mux", 10) == 0) 
  {
  	pREPLY.pScribe.media[0].fMultiplexRTCPWithRTP = true;
    return true;
  }
  else
  {
  	pREPLY.pScribe.media[0].fMultiplexRTCPWithRTP = false;
  	return false;
  }

}
bool cRTSPprotocol::parseSDPAttribute_control(char const* sdpLine)
{
  bool parseSuccess = false;
  char* controlPath = strdup(sdpLine); // ensures we have enough space
  if (sscanf(sdpLine, "a=control: %s", controlPath) == 1) 
  {
		parseSuccess = true;
		if(pREPLY.pScribe.media[0].fControlPath)
		{
			free(pREPLY.pScribe.media[0].fControlPath);
		}

		pREPLY.pScribe.media[0].fControlPath = strdup(controlPath);
  }
  free(controlPath);
  return parseSuccess;
}


int cRTSPprotocol::parseSDP(char const* sdpDescription)
{//参考live555 MediaSession.cpp initializeWithSDP


	char const* sdpLine = sdpDescription;
	char const* nextSDPLine;
	while (1) 
	{
		//跳过m=之前的信息，
	    if (!parseSDPLine(sdpLine, nextSDPLine)) return false;
	    if (sdpLine[0] == 'm') break;
	    sdpLine = nextSDPLine;
	    if (sdpLine == NULL) break; // there are no m= lines at all
	#if 0 // do not care 
	    // Check for various special SDP lines that we understand:
	    if (parseSDPLine_s(sdpLine)) continue;
	    if (parseSDPLine_i(sdpLine)) continue;
	    if (parseSDPLine_c(sdpLine)) continue;
	    if (parseSDPAttribute_control(sdpLine)) continue;
	    if (parseSDPAttribute_range(sdpLine)) continue;
	    if (parseSDPAttribute_type(sdpLine)) continue;
	    if (parseSDPAttribute_source_filter(sdpLine)) continue;
	#endif
   }

	while (sdpLine != NULL) 
	{
	 if (!parseSDPLine(sdpLine, nextSDPLine)) return false;
     if (sdpLine[0] == 'm')
     {
    	if( catchVideoMedinSession(sdpLine) )
    	{
    		 while (1) {
		      sdpLine = nextSDPLine;
		      if (sdpLine == NULL) break; // we've reached the end
		      if (!parseSDPLine(sdpLine, nextSDPLine)) return False;

		      if (sdpLine[0] == 'm') break; // we've reached the next subsession

		      // Check for various special SDP lines that we understand:
		      if (parseSDPAttribute_rtcpmux(sdpLine)) continue;
		      if (parseSDPAttribute_control(sdpLine)) continue;
			  #if 0//其他信息目前不管
			  if (subsession->parseSDPLine_c(sdpLine)) continue;
		      if (subsession->parseSDPLine_b(sdpLine)) continue;
		      if (subsession->parseSDPAttribute_rtpmap(sdpLine)) continue;
		      if (subsession->parseSDPAttribute_range(sdpLine)) continue;
		      if (subsession->parseSDPAttribute_fmtp(sdpLine)) continue;
		      if (subsession->parseSDPAttribute_source_filter(sdpLine)) continue;
		      if (subsession->parseSDPAttribute_x_dimensions(sdpLine)) continue;
		      if (subsession->parseSDPAttribute_framerate(sdpLine)) continue;
			  #endif
    		 }

     		 // (Later, check for malformed lines, and other valid SDP lines#####)
   		 }
    	}

	   sdpLine = nextSDPLine;
  	  if (sdpLine == NULL) break; // there are no m= lines at all
    }
  
}



bool cRTSPprotocol::parseTransportParams(char const* paramsStr,
					 char*& serverAddressStr, portNumBits& serverPortNum,
					 unsigned char& rtpChannelId, unsigned char& rtcpChannelId)
{
//copy  from live555  RTSPClient.cpp

  // Initialize the return parameters to 'not found' values:
  serverAddressStr = NULL;
  serverPortNum = 0;
  rtpChannelId = rtcpChannelId = 0xFF;
  if (paramsStr == NULL) return False;  

  char* foundServerAddressStr = NULL;
  Boolean foundServerPortNum = False;
  portNumBits clientPortNum = 0;
  Boolean foundClientPortNum = False;
  Boolean foundChannelIds = False;
  unsigned rtpCid, rtcpCid;
  Boolean isMulticast = True; // by default
  char* foundDestinationStr = NULL;
  portNumBits multicastPortNumRTP, multicastPortNumRTCP;
  Boolean foundMulticastPortNum = False;

  // Run through each of the parameters, looking for ones that we handle:
  char const* fields = paramsStr;
  char* field = strdup(fields);
  while (sscanf(fields, "%[^;]", field) == 1) {
    if (sscanf(field, "server_port=%hu", &serverPortNum) == 1) {
      foundServerPortNum = True;
    } else if (sscanf(field, "client_port=%hu", &clientPortNum) == 1) {
      foundClientPortNum = True;
    } else if (strncasecmp(field, "source=", 7) == 0) {
     free( foundServerAddressStr);
      foundServerAddressStr = strdup(field+7);
    } else if (sscanf(field, "interleaved=%u-%u", &rtpCid, &rtcpCid) == 2) {
      rtpChannelId = (unsigned char)rtpCid;
      rtcpChannelId = (unsigned char)rtcpCid;
      foundChannelIds = True;
    } else if (strcmp(field, "unicast") == 0) {
      isMulticast = False;
    } else if (strncasecmp(field, "destination=", 12) == 0) {
      free(foundDestinationStr);
      foundDestinationStr = strdup(field+12);
    } else if (sscanf(field, "port=%hu-%hu", &multicastPortNumRTP, &multicastPortNumRTCP) == 2 ||
	       sscanf(field, "port=%hu", &multicastPortNumRTP) == 1) {
      foundMulticastPortNum = True;
    }

    fields += strlen(field);
    while (fields[0] == ';') ++fields; // skip over all leading ';' chars
    if (fields[0] == '\0') break;
  }
  free(field);

  // If we're multicast, and have a "destination=" (multicast) address, then use this
  // as the 'server' address (because some weird servers don't specify the multicast
  // address earlier, in the "DESCRIBE" response's SDP:
  if (isMulticast && foundDestinationStr != NULL && foundMulticastPortNum) {
    free(foundServerAddressStr);
    serverAddressStr = foundDestinationStr;
    serverPortNum = multicastPortNumRTP;
    return True;
  }
  if(foundDestinationStr) free(foundDestinationStr);

  // We have a valid "Transport:" header if any of the following are true:
  //   - We saw a "interleaved=" field, indicating RTP/RTCP-over-TCP streaming, or
  //   - We saw a "server_port=" field, or
  //   - We saw a "client_port=" field.
  //     If we didn't also see a "server_port=" field, then the server port is assumed to be the same as the client port.
  if (foundChannelIds || foundServerPortNum || foundClientPortNum) {
    if (foundClientPortNum && !foundServerPortNum) {
      serverPortNum = clientPortNum;
    }
    serverAddressStr = foundServerAddressStr;
    return True;
  }

  free(foundServerAddressStr);
  return False;
}
					 
int cRTSPprotocol::parse(char*buf,int buflen,REQUEST_TYPE request,unsigned cseq,int *extData)
{
	//这里参考 live555 RTSPClient.cpp handResponseBytes();

	unsigned respondCode =0;
    char const* sessionParamsStr = NULL;
	char const* transportParamsStr = NULL;
	char const* scaleParamsStr = NULL;
    char const* speedParamsStr = NULL;
    char const* rangeParamsStr = NULL;
    char const* rtpInfoParamsStr = NULL;
	char const* contentParamsStr = NULL;
	int ret = RET_SUCESS;
	
    char* headerDataCopy = (char*)malloc(buflen);
	do{
		strncpy(headerDataCopy, buf, buflen);
		 headerDataCopy[buflen-1] = '\0';
		//先解析 respondCode

		 char* lineStart;
		 char* nextLineStart = headerDataCopy;
		 do {
			lineStart = nextLineStart;
			nextLineStart = getLine(lineStart);
		  } while (lineStart[0] == '\0' && nextLineStart != NULL); // skip over any blank lines at the start

		  if (RET_ERR == parseResponseCode(lineStart, &respondCode))
		  {
				DEBUG_ERR("parseErro \n");
				ret = RET_ERR;
				break;
		  }

		while(1)
		{//空行跳出
#if 1
			lineStart = nextLineStart;
			if (lineStart == NULL) break;
			
			nextLineStart = getLine(lineStart);
			if (lineStart[0] == '\0') break; // this is a blank line
			char const* headerParamsStr;

		//for any thing you interesed
			if (checkForHeader(lineStart, "CSeq:", 5, &headerParamsStr)) 
			{
				unsigned icseq =0;
			  	if (sscanf(headerParamsStr, "%u", &icseq) != 1 || cseq <= 0) 
				{
			   		DEBUG_WARN("Bad \"CSeq:\" header: %s", lineStart);
					ret = RET_ERR;
			   		 break;
			  	}

				if(icseq != cseq)
				{
					DEBUG_ERR("Bad CSeq!!!");
					ret = RET_ERR;
					break;
				}
			}
			else if(checkForHeader(lineStart, "Session:", 8, &sessionParamsStr))
			{
			}
			else if (checkForHeader(lineStart, "Transport:", 10, &transportParamsStr))
			{
			}
			else if (checkForHeader(lineStart, "Scale:", 6, &scaleParamsStr))
			{
			}
			else if(checkForHeader(lineStart, "Speed:",6,&speedParamsStr))
			{
			}
			else if(checkForHeader(lineStart, "Range:",6,&rangeParamsStr))
			{
			}
			else if (checkForHeader(lineStart, "RTP-Info:", 9, &rtpInfoParamsStr))
			{
			}
			else if(checkForHeader(lineStart, "Content-Length:", 15, &contentParamsStr))
			{
#if 0
			//检查conten 数据是否完全读进来
				unsigned contentLength =0;
				if(sscanf(contentParamsStr, "%u", &contentLength) != 1) 
				{
					ret = RET_ERR;
					DEBUG_ERR("parse erro:%s\n",lineStart);
					break;
				}
				bool bEnd;
				char *split1 = NULL;
				char *split12 = NULL;
				split1 = findBlankLine(nextLineStart, buflen - (headerDataCopy-nextLineStart),&bEnd);
				if(split1 == NULL)
				{
					ret = RET_ERR;
					DEBUG_ERR("parse erro,data too short \n");
					break;
				}
				else if(buflen - (split1+2-headerDataCopy))
				{
					DEBUG_INFO3("not end bulenleft :%d \n",buflen - (split1+2-headerDataCopy));
					split12 = findBlankLine(split1+2, buflen - (split1+2-headerDataCopy),&bEnd);
				}

				int haveReadConten = buflen - (split1+2 - headerDataCopy);
				DEBUG_WARN("haveReadConten:%d contentLength %d\n",haveReadConten,contentLength);
				if(NULL == split12 && contentLength > 0)
				{
					ret = RET_ERR_NEEDMOREDATA;
					if(extData)
					{
						*extData = contentLength - haveReadConten;
					}
					
					DEBUG_WARN("NEED more data :%d \n",*extData);
					break;
				}

				#endif
			}
#endif
		 }


		if(ret != RET_SUCESS)
		{
			break;
		}
		
		pREPLY.respondCode = respondCode;
		{ // Do special-case response handling for some commands:
			switch(request)
			{
				case REQUEST_DESCRIBE:
					{
						memset(&pREPLY.pScribe.media[0],0,sizeof(pREPLY.pScribe.media[0]));
						lineStart = nextLineStart;
						if (lineStart == NULL)
						{
							DEBUG_ERR("RPLY SDP info erro\n");
							ret = RET_ERR;
							break;
						}
						else
						{
							ret = parseSDP(lineStart);
							break;
						}
					}
					break;
				case REQUEST_OPTION:
					{
						DEBUG_INFO2("OPTION ret:%d \n",ret);
					}
					break;
				case REQUEST_SETUP:
					{//主要是获取 serverPortNum;
						memset(&pREPLY.pSetup,0,sizeof(pREPLY.pSetup));
					 	char* serverAddressStr = NULL;
    				 	portNumBits serverPortNum;
    				 	unsigned char rtpChannelId, rtcpChannelId;
						if( parseTransportParams(transportParamsStr, serverAddressStr, serverPortNum, rtpChannelId, rtcpChannelId))
						{
							//需要手动释放 部分内存 ,一般都是 0.0.0.0 ,无效
							if(serverAddressStr) free(serverAddressStr);
							
							pREPLY.pSetup.serverPortNum = serverPortNum;
						}
						else
						{
							ret = RET_ERR;
							DEBUG_ERR("parseTransportParams err\n");
							break;
						}
						 if (sessionParamsStr == NULL || sscanf(sessionParamsStr, "%[^;]", pREPLY.pSetup.fSessionId) != 1) 
						 {
							ret = RET_ERR;
							DEBUG_ERR("parse sessionParma err \n");
						    break;
						 }
						 
					}
					break;
				case REQUEST_PLAY:
					{
					}
					break;
				case REQUEST_PAUSE:
					{
					
					}
					break;
				case REQUEST_TEARDOWN:
					{
					
					}
					break;
			}
		}
		
		if(respondCode != 200)
		{
			//不做处理
			DEBUG_ERR("server reply err:respondCode: %d  !!!!!!!!!\n\n",respondCode);
			ret = RET_ERR;
			break;
		}
		else
		{
			ret = RET_SUCESS;
			break;
		}
		
	}
	while(0);
	free(headerDataCopy);

	DEBUG_INFO3("parse ret:%d \n",ret);

	return ret;
}

