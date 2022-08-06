#ifndef __UDPOPER_H__
#define __UDPOPER_H__
#include"globaltype.h"
#include"ihandler.h"


class UdpOper : public I_FdOper {
public:
    UdpOper();
    ~UdpOper();

    Void setParam(I_Handler* handler, const Void* addr, Int32 addrlen);

    Int32 readFd(PollItem* item);
    Int32 writeFd(PollItem* item);
    Int32 dealFd(PollItem* item);

private:
    I_Handler* m_handler;
    
    Int32 m_peer_addr_len;
    char m_peer_addr[DEF_ADDR_SIZE];
    
    static char m_udp_buf[DEF_UDP_RECV_BUF_SIZE];
};

#endif

