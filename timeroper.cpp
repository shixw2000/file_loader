#include"timeroper.h"
#include"sockutil.h"
#include"ticktimer.h"


TimerOper::TimerOper() {
    m_timer = NULL;
}

TimerOper::~TimerOper() {
}

Void TimerOper::setParam(TickTimer* timer) {
    m_timer = timer;
}

Int32 TimerOper::readFd(PollItem* item) {
    Int32 ret = 0;
    Uint32 cnt = 0;

    ret = readEvent(item->m_fd, &cnt);
    if (0 == ret) {
        m_timer->tick(cnt);
    }

    return ret;
}


