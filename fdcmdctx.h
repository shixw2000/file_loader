#ifndef __FDCMDCTX_H__
#define __FDCMDCTX_H__
#include<list>
#include"cthread.h"
#include"cmdctx.h"


class Engine;

class FdCmdCtx : public CmdCtx, public I_Element {
    typedef std::list<Engine*> typeList;
    typedef typeList::iterator typeListItr;
    
public:
    FdCmdCtx();
    virtual ~FdCmdCtx();

    Int32 start();
    Void stop();
    
    virtual Bool notify(Void* msg);

    virtual Bool emerge(Void* msg);

    virtual Void addRunList(Engine*);
    virtual Void delRunList(Engine*); 
    
    virtual Void read(Int32 fd);

private:
    Void run();

    Void doTasks();

private:
    Bool m_has_event;
    typeList m_run_list;
};

#endif

