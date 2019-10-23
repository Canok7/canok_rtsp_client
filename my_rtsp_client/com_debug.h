#ifndef __hearder_com_debug__
#define __hearder_com_debug__
#include<stdio.h>


#define RET_SUCESS 0
#define RET_ERR  -1
#define RET_ERR_NEEDMOREDATA -2


//从 第0开始算，start
#define LEFT_CHAR_BIT_GET_RAW(pData,start,len) ( (u_int8_t)(pData)[0] & ((1<<(8-start)) - (1<<(8-start-len))) )
#define LEFT_CHAR_BIT_GET(pData,start,len) (LEFT_CHAR_BIT_GET_RAW(pData,start,len)>>(8-start-len))

#define RIGHT_CHAR_BIT_GET(pData,start,len) ( ((u_int8_t)(pData)[0]>>(start)) & ((0x01<<(len)) -1) )

#define LITTLE_MAKE_UNINT16(pData) (((u_int8_t)(pData)[0]<<8)  | ((u_int8_t)(pData)[1]))
#define LITTLE_MAKE_UNINT32(pData) (((u_int8_t)(pData)[0]<<24) | ((u_int8_t)(pData)[1]<<16) | ((u_int8_t)(pData)[2]<<8) | (u_int8_t)(pData)[3])

#ifndef DEBUG_LEVE
#define DEBUG_LEVE 3
#endif

#define DEBUG_WARN(fmt, args...) do {printf("WARN![%s,%s,%d]",__FILE__,__FUNCTION__,__LINE__);printf(fmt,##args);} while(0)
#define DEBUG_ERR(fmt, args...) do {printf("ERR!!!![%s,%s,%d]",__FILE__,__FUNCTION__,__LINE__);printf(fmt,##args);} while(0)

#if DEBUG_LEVE >=3
#define DEBUG_INFO1(fmt, args...) do {printf("[%s,%s,%d]",__FILE__,__FUNCTION__,__LINE__);printf(fmt,##args);} while(0)
#define DEBUG_INFO2(fmt, args...) do {printf("[%s,%s,%d]",__FILE__,__FUNCTION__,__LINE__);printf(fmt,##args);} while(0)
#define DEBUG_INFO3(fmt, args...) do {printf("[%s,%s,%d]",__FILE__,__FUNCTION__,__LINE__);printf(fmt,##args);} while(0)
#elif DEBUG_LEVE ==2
#define DEBUG_INFO1(fmt, args...) do {printf("[%s,%s,%d]",__FILE__,__FUNCTION__,__LINE__);printf(fmt,##args);} while(0)
#define DEBUG_INFO2(fmt, args...) do {printf("[%s,%s,%d]",__FILE__,__FUNCTION__,__LINE__);printf(fmt,##args);} while(0)
#define DEBUG_INFO3(fmt, args...)   
#elif DEBUG_LEVE ==1
#define DEBUG_INFO1(fmt, args...) do {printf("[%s,%s,%d]",__FILE__,__FUNCTION__,__LINE__);printf(fmt,##args);} while(0)
#define DEBUG_INFO2(fmt, args...)  
#define DEBUG_INFO3(fmt, args...)  
#endif
//还未实现

/*
* 最好是单例模式，互斥同步, 用宏定义重新包装 接口
*/
class CDebugInfo
{
	public: 
		CDebugInfo(char infoLeve=-1);
		~CDebugInfo();
		void debug_warn();
		void debug_err();
		void debug_info_leve1();
		void debug_info_leve2();
		void debug_info_leve3();
	private:
		//info leve, 
		//-1 :all 
		//0 : warn and err,
		//3 : info3 info2 info 1 , warn and err,
		//2 : info2 info1 , warn and err,
		//1 : info1, warn and err
		char m_infoLeve;
};
#endif
