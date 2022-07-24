#ifndef __CMDCTX_H__
#define __CMDCTX_H__
#include<map>
#include"globaltype.h"
#include"filectx.h"
#include"queworker.h"
#include"eventmsg.h"


class TickTimer;
class Engine;

class CmdCtx : public QueWorker {  
    struct KeyCmp {
        bool operator()(const Char* s1, const Char* s2);
    };
    
    typedef std::map<const char*, Engine*, KeyCmp> typeTab;
    typedef typeTab::iterator typeTabItr;    
    
public:
    CmdCtx();
    virtual ~CmdCtx();

    Int32 start();
    Void stop();

    Void setTimer(TickTimer* timer) {
        m_timer = timer;
    } 

    Uint32 now() const;

    inline void setTest(CmdCtx* test) {
        m_test = test;
    }

    Int32 uploadFile(EvMsgStartUpload* msg);
    Int32 downloadFile(EvMsgStartDownload* msg);
    
    Int32 recvPkg(Void* msg);
    Int32 sendPkg(Void* msg);

    Int32 transPkg(Void* msg);

    inline TaskConfType* getConf() const { return m_data; }

    Void* creatTimer(QueWorker* que, Int32 type); 
    Void startTimer(Void* id, Uint32 tick);
    Void stopTimer(Void* id);    
    Void resetTimer(Void* id, Uint32 tick);
    Void delTimer(Void* id); 

    virtual Void addRunList(Engine*) = 0;
    virtual Void delRunList(Engine*) = 0;

    Void test_01(); 
    Void test_02();

protected:
    virtual Void dealEvent(Void* msg); 
    Int32 acceptUpload(Void* msg);
    Int32 acceptDownload(Void* msg);

    static Void activeTimer(Void* p1, Void* p2);

private:
    CmdCtx* m_test; 
    TickTimer* m_timer;
    TaskConfType* m_data;
    typeTab m_container;
};

#endif

