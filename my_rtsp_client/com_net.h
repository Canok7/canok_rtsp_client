#ifndef __header_com_net__
#define __header_com_net__
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>


//typedef char int8_t;
typedef unsigned char u_int8_t;

typedef short int16_t;
typedef unsigned short u_int16_t;

typedef int int32_t;
typedef unsigned int u_int32_t;


int udp_net_setup();
int udp_connect(int socket,const char*host, int port );

int udp_get_port(int socket, int16_t *port);


int tcp_net_conncet(const char *psz_host,int i_port);
int readSocket(  int socket,  char* buffer, unsigned bufferSize,
	        struct sockaddr *fromAddress = NULL);

int writeSocket(int socket, char* buffer, unsigned bufferSize,
			struct sockaddr* destAddress = NULL); 

int closeSocket(int socket);

bool makeSocketNonBlocking(int sock);

bool makeSocketBlocking(int sock, unsigned writeTimeoutInMilliseconds);

#endif
