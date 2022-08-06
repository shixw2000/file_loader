#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include"stdinoper.h"
#include"eventmsg.h"
#include"msgcenter.h"
#include"filepoll.h"


StdinOper::StdinOper() {
    m_dst = -1;
    m_poll = NULL;
}

StdinOper::~StdinOper() {
}

Void StdinOper::setParam(Int32 dst, FilePoll* poll) {
    m_dst = dst;
    m_poll = poll;
}

Int32 StdinOper::readFd(PollItem* item) {
    Int32 ret = 0;
    char buf[0x400] = {0};
    Int32 size = 0;
    FILE* fp = NULL;
    EvMsgTransData* pReq = NULL;

    fp = fdopen(item->m_fd, "rb");
    fgets(buf, sizeof(buf), fp);
    
    LOG_INFO("Enter a size:");
    ret = fscanf(fp, "%d", &size);
    if (1 == ret) {
        LOG_INFO("==size=%d|", size);
    } else {
        LOG_INFO("==read invalid|");
        return -1;
    }

    fgets(buf, sizeof(buf), fp);

    LOG_INFO("Enter a msg:");
    pReq = MsgCenter::creatMsg<EvMsgTransData>(CMD_TRANS_DATA_BLK, size);
    pReq->m_buf_size = size;
    fgets(pReq->m_buf, size, fp);
    MsgCenter::notify(pReq, &item->m_rcv_msgs);
    
    return 0;
}

Int32 StdinOper::dealFd(PollItem* item) {
    Int32 ret = 0;
    list_node* pos = NULL;
    list_node* n = NULL;
    Void* msg = NULL;

    list_for_each_safe(pos, n, &item->m_rcv_msgs) {
        list_del(pos, &item->m_rcv_msgs);
                
        msg = MsgCenter::entry<Void>(pos);
        
        m_poll->addSendQue(m_dst, msg);
    }
    
    return ret; 
}

