#include "com_debug.h"
#include "com_net.h"

#include <unistd.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <signal.h>

#include <errno.h>  
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include"memwatch.h"

int udp_net_setup()
{//udp 可以直接用recevfrom  指明从哪个主机和端口接受，但是每次都要填写地址信息
//所以udp可以用connet 确定好服务端地址端口
//同样也可以先用bind绑定好自己客户端的端口地址，避免自己调用send的时候系统给客户端自己分配一个端口
	int sockfd;
	struct sockaddr_in cli_addr;

	//close_on_exec。当父进程打开文件时，只需要应用程序设置FD_CLOSEXEC标志位，
	//则当fork后exec其他程序的时候，内核自动会将其继承的父进程FD关闭,避免子进程也同时操作，造成意外
	//

    //vlc 和live555的socket都有 |SOCK_CLOEXEC ， 解释如上
	sockfd = socket(AF_INET,SOCK_DGRAM | SOCK_CLOEXEC,0);
	if(sockfd < 0){
		DEBUG_ERR("Fail to socket\n");
		return -1;
	}

	//屏蔽掉SIGPIPE 消息
  	//  setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &(int){ 1 }, sizeof (int));
	int data =1;
	setsockopt(sockfd, SOL_SOCKET, MSG_NOSIGNAL, &data, sizeof (int));


	

	cli_addr.sin_family = AF_INET;
	#if 1//这里使用内核自动分配的端口
	cli_addr.sin_port   = 0;//!!!!!!!!!!!!
	#else

	cli_addr.sin_port   = htons(8554);//!!!!!!!!!!!!
	#endif
	cli_addr.sin_addr.s_addr = INADDR_ANY;
	if( bind(sockfd, (struct sockaddr*)&cli_addr, sizeof(struct sockaddr_in)) < 0)
	{
		//当前使用默认阻塞方式。
		DEBUG_ERR("Fail to bind :INADDR_ANY:0 err:%d %s\n",errno,strerror(errno));
		close(sockfd);
		return -1;
	}

	return sockfd;

}

int udp_connect(int sockfd,const char*host, int port)
{
	
	if(sockfd <= -1)
	{
		return RET_ERR;
	}
	
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port   = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(host);

	DEBUG_INFO3("connect host:%s,port:%d\n",host,port);
	if(connect(sockfd,(struct sockaddr *)&server_addr,sizeof(struct sockaddr_in)) < 0)
	{	
		//当前使用默认阻塞方式。
		DEBUG_ERR("Fail to connect :%s:%d err:%d %s\n",host,port,errno,strerror(errno));
		close(sockfd);
		return RET_ERR;
	}
	return RET_SUCESS;
}
//in net byte order
int udp_get_port(int socket, int16_t *port)
{//参考live555 GroupsockHelper.cpp getSourcePort

  sockaddr_in test; 
  test.sin_port = 0;
  socklen_t len = sizeof(test);
  if (getsockname(socket, (struct sockaddr*)&test, &len) < 0) 
  {
	  DEBUG_ERR("Fail to getsockname err:%d %s\n",errno,strerror(errno));
  	  return RET_ERR;
  }
  
  DEBUG_INFO3("GETCLIENT_UDP_PORT test.sin_port %d \n",test.sin_port);
  //*port = ntohs(test.sin_port);
  *port = test.sin_port;
  DEBUG_INFO3("GETCLIENT_UDP_PORT *port %d \n",*port);
  return RET_SUCESS;
}

//服务器端 ip，和port, 本地的port会自动分配
int tcp_net_conncet(const char *psz_host,int i_port)
{
	int sockfd;
	struct sockaddr_in server_addr;

	//close_on_exec。当父进程打开文件时，只需要应用程序设置FD_CLOSEXEC标志位，
	//则当fork后exec其他程序的时候，内核自动会将其继承的父进程FD关闭,避免子进程也同时操作，造成意外
	//

    //vlc 和live555的socket都有 |SOCK_CLOEXEC ， 解释如上
	sockfd = socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0);
	if(sockfd < 0){
		DEBUG_ERR("Fail to socket\n");
		return -1;
	}

	//屏蔽掉SIGPIPE 消息
  	//  setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &(int){ 1 }, sizeof (int));
	int data =1;
	setsockopt(sockfd, SOL_SOCKET, MSG_NOSIGNAL, &data, sizeof (int));

	// int curFlags = fcntl(sockfd, F_GETFL, 0);
    //	 fcntl(sockfd, F_SETFL, curFlags|O_NONBLOCK); //

	server_addr.sin_family = AF_INET;
	server_addr.sin_port   = htons(i_port);
	server_addr.sin_addr.s_addr = inet_addr(psz_host);
	if(connect(sockfd,(struct sockaddr *)&server_addr,sizeof(struct sockaddr_in)) < 0)
	{	//这里可以实现自定义超时时间，不过比较麻烦:
		//https://www.cnblogs.com/alantu2018/p/8469502.html
		//https://blog.csdn.net/CodeHeng/article/details/44625495
		
		//当前使用默认阻塞方式。
		DEBUG_ERR("Fail to connect :%s:%d err:%d %s\n",psz_host,i_port,errno,strerror(errno));
		close(sockfd);
		return -1;
	}

	return sockfd;
	
}


int readSocket(  int socket,   char* buffer, unsigned bufferSize,
	       struct sockaddr *fromAddress)
{
	socklen_t addrlen = sizeof( struct sockaddr);
	int bytesRead = recvfrom(socket, buffer, bufferSize, 0,
                       fromAddress, &addrlen);
	if(bufferSize < 0)
	{//erro
		DEBUG_ERR("err:%d:%s\n",errno,strerror(errno));
	}
	return bytesRead;
}
		   
int writeSocket(int socket,char* buffer, unsigned bufferSize,
				struct sockaddr * destAddress) 
{
	socklen_t addrlen = sizeof( struct sockaddr);
	int bytesSent = sendto(socket, (char*)buffer, bufferSize, 0,
			  destAddress, addrlen);
	if (bytesSent != (int)bufferSize) 
	{//erro
		DEBUG_ERR("writeSocket erro %d/%d \n",bytesSent,bufferSize);
	}
	return bytesSent;
}

int closeSocket(int socket)
{
	return close(socket);
}
bool makeSocketNonBlocking(int sock) 
{
  int curFlags = fcntl(sock, F_GETFL, 0);
  return fcntl(sock, F_SETFL, curFlags|O_NONBLOCK) >= 0;
}


bool makeSocketBlocking(int sock, unsigned writeTimeoutInMilliseconds)
{
  bool result;
#if defined(__WIN32__) || defined(_WIN32)
  unsigned long arg = 0;
  result = ioctlsocket(sock, FIONBIO, &arg) == 0;
#elif defined(VXWORKS)
  int arg = 0;
  result = ioctl(sock, FIONBIO, (int)&arg) == 0;
#else
  int curFlags = fcntl(sock, F_GETFL, 0);
  result = fcntl(sock, F_SETFL, curFlags&(~O_NONBLOCK)) >= 0;
#endif

  if (writeTimeoutInMilliseconds > 0) {
#ifdef SO_SNDTIMEO
#if defined(__WIN32__) || defined(_WIN32)
    DWORD msto = (DWORD)writeTimeoutInMilliseconds;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&msto, sizeof(msto) );
#else
    struct timeval tv;
    tv.tv_sec = writeTimeoutInMilliseconds/1000;
    tv.tv_usec = (writeTimeoutInMilliseconds%1000)*1000;

	//send block and recv block
     setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof tv);
	 setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO, (char *)&tv, sizeof tv);
#endif
#endif
  }

  return result;
}

unsigned getReceiveBufferSize( int socket)
{
	unsigned curSize;
	socklen_t  sizeSize = sizeof(curSize);
	if (getsockopt(socket, SOL_SOCKET, SO_RCVBUF,(char*)&curSize, &sizeSize) < 0)
	{
		DEBUG_ERR("get buffsize erro\n");
		return 0;
	}
	return curSize;
}


unsigned increaseReceiveBufferTo(int socket, unsigned requestedSize)
{
	// First, get the current buffer size.  If it's already at least
	// as big as what we're requesting, do nothing.
	unsigned curSize = getReceiveBufferSize(socket);

	// Next, try to increase the buffer to the requested size,
	// or to some smaller size, if that's not possible:
	while (requestedSize > curSize)
	{
		socklen_t sizeSize = sizeof(requestedSize);
		if (setsockopt(socket, SOL_SOCKET, SO_RCVBUF,(char*)&requestedSize, sizeSize) >= 0)
		{
			// success
			return requestedSize;
		}
		requestedSize = (requestedSize+curSize)/2;
	}
	return getReceiveBufferSize(socket);
}



