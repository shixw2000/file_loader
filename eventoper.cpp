#include"eventoper.h"
#include"msgcenter.h"
#include"sockutil.h"
#include"eventmsg.h"
#include"filepoll.h"


EventOper::EventOper() {
    m_obj = NULL;
}

EventOper::~EventOper() {
}

Void EventOper::setParam(I_Obj* obj) {
    m_obj = obj;
}

Int32 EventOper::readFd(PollItem* item) {
    Int32 ret = 0;
    Uint32 cnt = 0;

    ret = readEvent(item->m_fd, &cnt);
    if (0 == ret) {        
        m_obj->event(cnt);
    }

    return ret;
}

Int32 EventOper::writeFd(PollItem* item) {
    list_node* pos = NULL;
    Void* msg = NULL;

    while (!list_empty(&item->m_snd_msgs)) {
        pos = LIST_FIRST(&item->m_snd_msgs);
        list_del(pos, &item->m_snd_msgs);
        
        msg = MsgCenter::entry<Void>(pos);
        MsgCenter::freeMsg(msg);

        writeEvent(item->m_fd);
    } 

    item->m_more_write = FALSE;
    return 0;
}


SignalOper::SignalOper() {
    m_obj = NULL;
    m_fd = -1;
    m_has_event = FALSE;
}

SignalOper::~SignalOper() {
}

Void SignalOper::setParam(Int32 fd, I_Obj* obj) {
    m_obj = obj;
    m_fd = fd;
}

Void SignalOper::signal() {
    Int32 ret = 0;
    
    if (!m_has_event) {
        ret = writeEvent(m_fd);
        if (0 == ret) {
            m_has_event = TRUE;
        }
    }
}

Int32 SignalOper::readFd(PollItem* item) {
    Int32 ret = 0;
    Uint32 cnt = 0;

    ret = readEvent(item->m_fd, &cnt);
    if (0 == ret) {
        if (m_has_event) {
            m_has_event = FALSE;
        }
        
        m_obj->event(cnt); 
    }
    
    return ret;
}


