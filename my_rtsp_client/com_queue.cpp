#include<stdio.h>
#include<stdlib.h>
#include <string.h>

#include "com_debug.h"
#include "com_queue.h"
//#include "jnilog.h"

#include"memwatch.h"

#define LOOP_ADD(i,r) ((i)+1)>=(r)?0:(i+1)

/*
#if 1
//#define DEBUG(fmt, args...)		do {printf("[%s,%s,%d]",__FILE__,__FUNCTION__,__LINE__);printf(fmt,##args);} while(0)
#define DEBUG(...)  __android_log_print(ANDROID_LOG_INFO,"Queue","[%d %s]",__LINE__,__FUNCTION__);__android_log_print(ANDROID_LOG_INFO,MY_LOG_TAG,__VA_ARGS__)
#else
#define DEBUG(fmt, args...)  
#endif
*/
CQueue::CQueue(int frames,int sigleFrameLen):
maxFrames(frames),
mframeLen(sigleFrameLen),
mInindex(0),
mOutindex(0),
mEffectives(0)
{	
	que = (QUEUE_NODE *)malloc(maxFrames*sizeof(QUEUE_NODE));
	uint8_t *pData = (uint8_t *)malloc(maxFrames*mframeLen);
	if(que == NULL || pData == NULL)
	{
		printf("erro to malloc\n");
	}
	
	pthread_mutexattr_t mutextattr;
	pthread_mutexattr_init(&mutextattr);
	// 设置互斥锁在进程之间共享
	pthread_mutexattr_setpshared(&mutextattr, PTHREAD_PROCESS_SHARED);
	
	int i =0;
	for(i=0;i<maxFrames;i++)
	{// to init node
		pthread_mutex_init(&que[i].mlock, &mutextattr);
		que[i].data = pData + mframeLen*i;
		//DEBUG_INFO3("que[%d].data %p \n",i,que[i].data);
	}
	pthread_mutex_init(&mIndexlock, &mutextattr);
	sem_init(&mSem, 0, 0);

}

CQueue::~CQueue()
{
	int i =0;
	for(i=0;i++;i<maxFrames)
	{
		pthread_mutex_destroy(&que[i].mlock);
	}
	pthread_mutex_destroy(&mIndexlock);
	sem_destroy(&mSem);
	if(que != NULL)
	{
		free(que[0].data);
		free(que);
		que = NULL;
	}

}
int CQueue::pop(uint8_t *data,int len)
{
	//DEBUG_INFO1("in %d out %d len %d\n ",mInindex,mOutindex,len);
	
	//sem_wait(&mSem);
	sem_trywait(&mSem);
	int iout = mOutindex;
	
	pthread_mutex_t * ilock = &que[iout].mlock;
	pthread_mutex_lock(ilock);
	
	if(reduceEffectives() ==0)
	{	
		mEffectives = 0;
		pthread_mutex_unlock(ilock);
		DEBUG_INFO1("no data\n");
		return 0;
	}
	
	int copylen = MIN(len,que[iout].datalen);
	if(data != NULL)
	{
		memcpy(data,que[iout].data,copylen);
	}
	
	addOutindex();
	pthread_mutex_unlock(ilock);
	return copylen;
}

int CQueue::push(uint8_t *data,int len)
{
	//DEBUG_INFO1("in %d out %d len %d\n ",mInindex,mOutindex,len);
	int iIn = mInindex;
	pthread_mutex_t * ilock = &que[iIn].mlock;
	
	pthread_mutex_lock(ilock);
	int copylen = MIN(len,mframeLen);
	
	if(data != NULL)
	{
		memcpy(que[iIn].data,data,copylen);
		que[iIn].datalen = copylen;
		if(copylen < len)
		{
			DEBUG_INFO1("Buffer single frame too short ! Buffer_frame_len:%d,data_len:%d \n ",mframeLen,len);
		}
	}
	
	addInindex();
	sem_post(&mSem);
	if(IncreaseEffectives() == maxFrames)
	{//覆盖老数据
		DEBUG_INFO1("cover  mEffectives %d ,maxFrames %d\n",mEffectives,maxFrames);
		//DEBUG_INFO1("cover  mInindex %d ,mOutindex %d\n",mInindex,mOutindex);
		sem_trywait(&mSem);
		addOutindex();
	}
	pthread_mutex_unlock(ilock);
	

	return copylen;

}

int CQueue::getbuffer(uint8_t **pdata,int *plen)
{
	int val=0;
	sem_wait(&mSem);
	int iout = mOutindex;

	pthread_mutex_t * ilock = &que[iout].mlock;
	pthread_mutex_lock(ilock);
	//DEBUG_INFO1("pop  mEffectives %d ,maxFrames %d\n",mEffectives,maxFrames);
	if(reduceEffectives() ==0 )
	{	
		*pdata = NULL;
		*plen =  0;
		pthread_mutex_unlock(ilock);
		DEBUG_INFO1("no data\n");
		return -1;
	}

	*pdata = que[iout].data;
	*plen =  que[iout].datalen;
	
	addOutindex();
	//DEBUG_INFO1("in %d out %d len %d unlock \n ",mInindex,mOutindex,len);
	//pthread_mutex_unlock(ilock);
	
	//DEBUG_INFO1("get index  %d \n",iout);
	return iout;
}
int CQueue::releasebuffer(int index)
{
	//DEBUG_INFO1("releas %d \n",index);
	if(0 <= index && index < maxFrames)
	{
		pthread_mutex_unlock(&que[index].mlock);
	}
	return 0;
}


void CQueue::addInindex()
{
	pthread_mutex_lock(&mIndexlock);
	mInindex = LOOP_ADD(mInindex,maxFrames);
	pthread_mutex_unlock(&mIndexlock);
}
void CQueue::addOutindex()
{
	pthread_mutex_lock(&mIndexlock);
	mOutindex = LOOP_ADD(mOutindex,maxFrames);
	pthread_mutex_unlock(&mIndexlock);
}

int CQueue::IncreaseEffectives()
{
	pthread_mutex_lock(&mIndexlock);
	int ret = mEffectives;
	mEffectives += 1;
	if(mEffectives > maxFrames)
	{	
		mEffectives = maxFrames;
	}
	pthread_mutex_unlock(&mIndexlock);
	return ret;
}

int CQueue::reduceEffectives()
{
	pthread_mutex_lock(&mIndexlock);
	
	int ret = mEffectives;
	mEffectives -= 1;
	if(mEffectives<0)
	{	
		mEffectives = 0;
	}
	pthread_mutex_unlock(&mIndexlock);
	return ret;
}


#if 0
int main()
{

	CQueue * pQueue = new CQueue(3,100);

	uint8_t buf1[128]="11111";
	uint8_t buf2[128]="22222";
	uint8_t buf3[128]="33333";
	uint8_t buf4[128]="44444";
	uint8_t buf5[128]="55555";
	uint8_t bufget[128]={0};
	
	int len = sizeof(bufget);
	pQueue->push(buf1,sizeof(buf1));
	#if 1
	pQueue->push(buf2,sizeof(buf2));
	pQueue->push(buf3,sizeof(buf3));
	pQueue->push(buf4,sizeof(buf4));
	pQueue->push(buf5,sizeof(buf5));

	pQueue->pop(bufget,len);
	printf("wang bufget :%s \n",bufget);

	pQueue->pop(bufget,len);
	printf("wang bufget :%s \n",bufget);
	
	pQueue->pop(bufget,len);
	printf("wang bufget :%s \n",bufget);
	
	pQueue->pop(bufget,len);
	printf("wang bufget :%s \n",bufget);

	
	pQueue->pop(bufget,len);
	printf("wang bufget :%s \n",bufget);


	//pQueue->push(buf1,sizeof(buf1));
	//len = sizeof(bufget);
	#endif
	//pQueue->pop(bufget,len);
	//printf("wang bufget :%s \n",bufget);
	
	delete pQueue;

	return 0;
}
#endif



