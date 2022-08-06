#ifndef __TASKSTAT_H__
#define __TASKSTAT_H__
/****
    * This file is used as concrete state machines. 
****/
#include"istate.h"


class I_Ctx;

class TaskFailEnd : public I_State {
public:
    explicit TaskFailEnd();
    virtual ~TaskFailEnd();
    
    virtual Void prepare(I_Ctx*);
    virtual Void process(Void*);
    virtual Void post();
};

class TaskSucessEnd : public I_State {
public:
    explicit TaskSucessEnd();
    virtual ~TaskSucessEnd();
    
    virtual Void prepare(I_Ctx*);
    virtual Void process(Void*);
    virtual Void post();
};


#endif

