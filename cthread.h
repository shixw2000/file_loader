#ifndef __CTHREAD_H__
#define __CTHREAD_H__
#include<vector>
#include"globaltype.h"


class I_Element {
public:
    I_Element() : m_fd(-1) {}
    virtual ~I_Element() {}
    
    virtual Void read(Int32 fd) = 0;

    Void setFd(Int32 fd) {
        m_fd = fd;
    }
    
    Int32 getFd() const {
        return m_fd;
    }

protected:
    Int32 m_fd;
};

class CThread {
    typedef std::vector<I_Element*> typeVec;
    typedef typeVec::iterator typeItr;
    
public:
    CThread();
    ~CThread();
    
    Int32 start();
    Void stop();
    
    Bool update(Bool more);

    void run(); 

    Void addEle(I_Element* ele);
    Void delEle(I_Element* ele);

private:
    volatile Bool m_running;
    typeVec m_vec;
};

Int32 creatTimerFd(Int32 ms);
Int32 creatEventFd();

Int32 writeEvent(int fd);
Int32 readEvent(Int32 fd);

#endif

