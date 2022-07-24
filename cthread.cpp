#include<sys/timerfd.h>
#include<sys/eventfd.h>
#include<poll.h>
#include<unistd.h>
#include"cthread.h"


CThread::CThread() {
    m_running = FALSE;
}

CThread::~CThread() {
}

Int32 CThread::start() {
    Int32 ret = 0; 
    
    m_running = TRUE; 
    return ret;
}

Void CThread::stop() {
    m_running = FALSE; 
}

Bool CThread::update(Bool more) {
    Int32 ret = 0;
    Int32 timeout = 0;
    Int32 nfds = 0;
    I_Element* pEle = NULL;
    struct pollfd fds[100];

    nfds = (Int32)m_vec.size();
    for (int i=0; i<nfds; ++i) {
        pEle = m_vec[i];
        
        fds[i].fd = pEle->getFd();
        fds[i].events = POLLIN;
        fds[i].revents = 0;
    }

    if (!more) {
        timeout = 100;
    }
    
    ret = poll(fds, nfds, timeout);
    if (0 < ret) {
        for (int i=0; i<nfds; ++i) {
            if (fds[i].revents & POLLIN) {
                for (int j=0; j<(Int32)m_vec.size(); ++j) {
                    pEle = m_vec[j];
                    if (fds[i].fd == pEle->getFd()) {
                        pEle->read(fds[i].fd);
                        break;
                    }
                } 
            }
        }

        return TRUE;
    } else {
        return FALSE;
    }
} 

void CThread::run() {
    Bool more = FALSE;
    
    while (m_running) {
        more = update(more);
    }
}

Void CThread::addEle(I_Element* ele) {
    m_vec.push_back(ele);
}

Void CThread::delEle(I_Element* ele) {
    for (typeItr itr=m_vec.begin(); itr!=m_vec.end(); ++itr) {
        if ((Int64)ele == (Int64)(*itr)) {
            m_vec.erase(itr);
            break;
        }
    }
}

Int32 writeEvent(int fd) {
    Int32 ret = 0;
    Int32 len = sizeof(Uint64);
    Uint64 val = 1;

    ret = write(fd, &val, len);
    if (ret == len) {
        return 0;
    } else {
        return -1;
    }
}

Int32 readEvent(int fd) {
    Int32 ret = 0;
    Int32 len = sizeof(Uint64);
    Uint64 val = 1;

    ret = read(fd, &val, len);
    if (ret == len) {
        return 0;
    } else {
        return -1;
    }
}

Int32 creatTimerFd(Int32 ms) {
    Int32 ret = 0;
    Int32 fd = -1;
    struct itimerspec value;

    fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (0 > fd) {
        return -1;
    }

    value.it_value.tv_sec = 0;
	value.it_value.tv_nsec = 1;
    value.it_interval.tv_sec = 0;
	value.it_interval.tv_nsec = ms * 1000000;
    ret = timerfd_settime(fd, 0, &value, NULL);
    if (0 != ret) {
        close(fd);
        return -1;
    }

    return fd;
}

Int32 creatEventFd() {
    Int32 fd = -1; 

    fd = eventfd(1, EFD_NONBLOCK);
    if (0 > fd) {
        return -1;
    } 
    
    return fd;
}

