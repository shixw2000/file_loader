#include"fdtimer.h"
#include"filetool.h"


FdTimer::FdTimer() {
}

FdTimer::~FdTimer() {
}

Int32 FdTimer::start() {
    Int32 ret = 0;
    Int32 fd = 0;

    ret = TickTimer::start();
    if (0 != ret) {
        return ret;
    }
    
    fd = creatTimerFd(100);
    if (0 <= fd) {
        setFd(fd);
        return 0;
    } else {
        return -1;
    } 
}

Void FdTimer::stop() {
    FileTool::closeFd(&m_fd);

    TickTimer::stop();
}

Void FdTimer::read(Int32 fd) {
    Int32 ret = 0;

    ret = readEvent(fd);
    if (0 == ret) {
        tick();
    }
}
