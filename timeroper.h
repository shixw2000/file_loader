#ifndef __TIMEROPER_H__
#define __TIMEROPER_H__
#include"globaltype.h"
#include"ihandler.h"


class TickTimer;

class TimerOper : public I_FdOper {
public:
    TimerOper();
    ~TimerOper();

    Void setParam(TickTimer* timer); 
    
    Int32 readFd(PollItem* item);
    Int32 writeFd(PollItem*) { return -1; }
    Int32 dealFd(PollItem*) { return -1; }

private:
    TickTimer* m_timer;
};


#endif

