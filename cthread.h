#ifndef __CTHREAD_H__
#define __CTHREAD_H__
#include"globaltype.h"


class I_Element {
public:
    virtual ~I_Element() {}
    virtual Int32 run() = 0;
};

class CThread {
public:
    CThread();
    virtual ~CThread();
    
    Int32 start();
    Void stop();

    Void setParam(I_Element* element);
  
private:
    I_Element* m_element;
    volatile Bool m_running;
};


#endif

