#ifndef __ISTATE_H__
#define __ISTATE_H__
#include"globaltype.h"


class I_Ctx;

/* interface of The state machine */
class I_State {
public:
    virtual ~I_State(){}
    
    /* Entrance of each state before msg dealing, 
        * You can change state inside if some condition
        * of the state not satisfied.
        */
    virtual Void prepare(I_Ctx*) = 0;
    
    /* msg handler */
    virtual Void process(Void*) = 0;

    /* Exit of a state, You can not change state inside this function */
    virtual Void post() {};
};

/* Interface of a factory by factory design mode */
class I_Factory {
public:
    virtual ~I_Factory() {}
    virtual I_State* creat(Int32 type) = 0;    
    virtual Void release(I_State*) = 0;
};

#endif

