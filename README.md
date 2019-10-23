
# canok_rtsp_client
rtsp client for learning rtsp, can get video stream of h264 loaded on  rtp-over-udp  
  
  
介绍：学习rtsp协议。 目前在ubuntu 上使用。 从零编写，代码结构清晰，注释详细。  
      当前支持 rtsp-rtp_over_udp, 拉取h264视频流，可以获取到h264裸流文件。  
   详细实现 rtsp tcp socket连接的建立，读写交互， RTSP 协议的解析。 RTP -udp socket的建立，rtp负载h264的解析，预留接口可以扩展支持其他流的解析。  

已知缺陷：  
      未建立rtcp  
      没有缓冲机制，对udp失序不做处理  

主要文件：  
  Makefile  
    
com_queue.cpp 缓冲队列的实现  
com_debug.cpp 调试信息控制  
com_net.cpp   socket 相关操作，connet,bind,设置阻塞模式等  
memwatch.cpp   用来检测内存的模块，可在makefile中关闭  
Pcapfile.cpp   pcap抓包文件的读取。用于模拟调试 
   
test_main.cpp   主程序demo  
   
cRTSPClient.cpp rtsp 会话  
cRTSPPprotocol.cpp  服务器回应rtsp信息的解析  
cRTPClient.cpp  rtp 会话  

cRTPParse.cpp  解析rtp负载  
cH264RTP.cpp    

运行test_main.cpp ,正常情况下可以在当前目录得到 live_out 输出文件，即h264裸流文件。  
rtsp_log 文件，记录有rtsp客户端发送的rtsp信息和收到的回应信息  
