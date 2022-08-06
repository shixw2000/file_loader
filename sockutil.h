#ifndef __SOCKUTIL_H__
#define __SOCKUTIL_H__
#include"globaltype.h"


Int32 creatUdpSock(const Char local_ip[], int local_port);

Int32 creatTcpSrv(const Char local_ip[], int local_port);

Int32 creatTcpCli(const Char peer_ip[], int peer_port);

Int32 setNonBlock(Int32 fd);
Int32 setReuse(Int32 fd);

Int32 creatAddr(const Char ip[], int port, Void* addr, Int32* plen);
Int32 parseAddr(const Void* addr, Char ip[], int* port);
Int32 getLocalInfo(Int32 fd, Char ip[], int* port);
Int32 getPeerInfo(Int32 fd, Char ip[], int* port);

Int32 connAddr(Int32 fd, const Void* addr, Int32 len);

Int32 closeHd(Int32 fd);

Int32 sendUdp(Int32 fd, const Void* buf, Int32 len,
    const Void* addr, Int32 addr_len);

Int32 recvUdp(Int32 fd, Void* buf, Int32 len, 
    Void* addr, int* addr_len); 

Int32 sendTcp(Int32 fd, const Void* buf, Int32 len);
Int32 recvTcp(Int32 fd, Void* buf, Int32 maxlen);
Int32 peekTcp(Int32 fd, Void* buf, Int32 maxlen);
Int32 acceptCli(Int32 fd);


Int32 creatTimerFd(Int32 ms);
Int32 creatEventFd();

Int32 writeEvent(int fd);
Int32 readEvent(Int32 fd, Uint32* pVal = NULL);

Void sysPause();

#endif

