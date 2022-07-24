#ifndef __FDTIMER_H__
#define __FDTIMER_H__
#include"cthread.h"
#include"ticktimer.h"


class FdTimer : public TickTimer, public I_Element {
public:
    FdTimer();
    ~FdTimer();

    Int32 start();
    Void stop();

    Void read(Int32 fd);
};

#endif

