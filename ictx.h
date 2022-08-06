#ifndef __ICTX_H__
#define __ICTX_H__
#include"globaltype.h"


class I_State;
class I_Factory;
class Engine;

/* The state machine context, driven by the input msg */
class I_Ctx { 
public:
    I_Ctx();
    virtual ~I_Ctx();

    /* used as a initializer function for preparation of dealing msg */
    virtual Int32 start();

    /* stop msg handler and do some resource recycle */
    virtual Void stop();

    /* this parameter of factory must be set before start */
    Void setFactory(I_Factory*);

    /* used by the state machine insiede for changing */
    void setState(Int32 type);
    
    /* the entrance of msg */
    void procEvent(Void* msgs); 

    /* notice a fail state */
    virtual Void fail(Int32 errcode) = 0;
    virtual Void reportResult() = 0;

private:
    Void transfer();

private:
    Int32 m_type; // the current state type 
    volatile Bool  m_has_chg; // notice the state changed
    I_State* m_state; // the current state, recycle by this class 
    I_Factory* m_factory; // used as a state creater, referenced by this class 
};

#endif

