#ifndef  __QUEUE_HEAD_H__
#define __QUEUE_HEAD_H__
#include <pthread.h>
#include <semaphore.h>

typedef unsigned char   uint8_t;     //无符号8位数

#define MIN(a,b) (a)<(b)?(a):(b)

typedef struct _QUEUE_NODE_
{
	pthread_mutex_t mlock;
	int datalen;
	uint8_t *data;
} QUEUE_NODE;



class CQueue
{
	public :
		CQueue(int frames,int sigleFrameLen);
		~CQueue();
		
		/*
		* \\no block
		*\\ pram: data, the dest data buffer, len , dest data buffer lenth,
		*\\ return : real  output data lenth
		*/
		int pop(uint8_t *data,int len);

		
		/*
		*\\ pram: data, the src data, len , the src data lenth
		*\\ return : real  input data lenth
		*/
		int push(uint8_t *data,int len);

		/*	
		* \\ breif :get the data buffer pointer whith block, Manual release is required after use
		*\\  param : **pdata , *plen :the queue data len
		*\\  return -1; no data to get. other the que index will be return ,
		*/
		int getbuffer(uint8_t **pdata,int *plen);
		
		/*
		*\\ breif : get back the que data buffer
		*\\ pram:the que index 
		*\\ return : 0
		*/
		int releasebuffer(int index);

	private:
		void addInindex();
		void addOutindex();
		int IncreaseEffectives();
		int reduceEffectives();
	private:
		int maxFrames;
		int mframeLen;
		QUEUE_NODE *que;
		int mInindex;
		int mOutindex;
		int mEffectives;
		sem_t mSem;

		pthread_mutex_t mIndexlock;
		
	
};
#endif

