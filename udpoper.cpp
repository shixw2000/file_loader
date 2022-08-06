#include<string.h>
#include"sockutil.h"
#include"udpoper.h"
#include"msgcenter.h"


char UdpOper::m_udp_buf[DEF_UDP_RECV_BUF_SIZE];

UdpOper::UdpOper() {
    m_handler = NULL;
    m_peer_addr_len = 0;
}

UdpOper::~UdpOper() {
}

Void UdpOper::setParam(I_Handler* handler, 
    const Void* addr, Int32 addrlen) { 
    m_handler = handler;
    m_peer_addr_len = addrlen;
    memcpy(m_peer_addr, addr, m_peer_addr_len);
}


/* return: 0: ok, -1: error */
Int32 UdpOper::readFd(PollItem* item) {
    Int32 rdlen = 0;
    Void* msg = NULL; 
    
    while (TRUE) {
        rdlen = recvUdp(item->m_fd, m_udp_buf, 
            DEF_UDP_RECV_BUF_SIZE, NULL, NULL);
        if (0 < rdlen) {
            msg = MsgCenter::dupUdp(m_udp_buf, rdlen);
            if (NULL != msg) {
                MsgCenter::notify(msg, &item->m_rcv_msgs);
            }

            continue;
        } else if (0 == rdlen) {
            return 0;
        } else {
            return -1;
        }
    }
}

/* return: 0: ok, -1: error */
Int32 UdpOper::writeFd(PollItem* item) {
    Int32 len = 0;
    Int32 sndlen = 0;
    list_node* pos = NULL;
    Void* msg = NULL;
    Char* psz = NULL;

    while (!list_empty(&item->m_snd_msgs)) {
        pos = LIST_FIRST(&item->m_snd_msgs);
        msg = MsgCenter::entry<Void>(pos);

        psz = MsgCenter::buffer(msg);
        len = MsgCenter::bufsize(msg);
        sndlen = sendUdp(item->m_fd, psz, len, m_peer_addr, m_peer_addr_len);
        if (sndlen == len) {
            list_del(pos, &item->m_snd_msgs);
            MsgCenter::freeMsg(msg);

            continue;
        } else if (0 == sndlen) {
            /* busy for send */
            item->m_can_write = FALSE;
            return 0;
        } else if (-2 == sndlen) {
            /* size or memory error */
            list_del(pos, &item->m_snd_msgs);
            MsgCenter::freeMsg(msg);

            continue;
        } else {
            return -1;
        }
    }

    item->m_more_write = FALSE;
    return 0;
}

Int32 UdpOper::dealFd(PollItem* item) {
    Int32 ret = 0;
    list_node* pos = NULL;
    list_node* n = NULL;
    Void* msg = NULL;

    list_for_each_safe(pos, n, &item->m_rcv_msgs) {
        list_del(pos, &item->m_rcv_msgs);
                
        msg = MsgCenter::entry<Void>(pos);
        m_handler->process(msg);
    }
    
    return ret; 
}

