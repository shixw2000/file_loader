#ifndef __EVENTOPER_H__
#define __EVENTOPER_H__
#include"globaltype.h"
#include"ihandler.h"


class EventOper : public I_FdOper {
public:
    EventOper();
    ~EventOper();

    Void setParam(I_Obj* obj); 
    
    Int32 readFd(PollItem* item);
    Int32 writeFd(PollItem* item);
    Int32 dealFd(PollItem*) { return -1; }

private:
    I_Obj* m_obj;
};


class SignalOper : public I_FdOper {
public:
    SignalOper();
    ~SignalOper();

    Void setParam(Int32 fd, I_Obj* obj);
    
    Void signal();

    Int32 readFd(PollItem* item);
    Int32 writeFd(PollItem*) { return -1; }
    Int32 dealFd(PollItem*) { return -1; } 

private:
    I_Obj* m_obj;
    Int32 m_fd;
    Bool m_has_event;
};


#endif

