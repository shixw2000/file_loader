#ifndef __CMDCTX_H__
#define __CMDCTX_H__
#include"globaltype.h"
#include"filectx.h"
#include"queworker.h"
#include"eventmsg.h"


class I_Comm;
class TickTimer;
class Engine; 

class CmdCtx : protected QueWorker { 
public:
    CmdCtx();
    virtual ~CmdCtx();

    void setParam(TickTimer* timer, I_Comm* comm);

    Int32 start();
    Void stop(); 

    void setSpeed(Int32 sndSpeed, Int32 rcvSpeed);
    
    Void run(); 
    
    Int32 sendPkg(Void* msg);
    Int32 transPkg(Void* msg);
    Int32 recvPkg(Void* msg);
    
    Void addRunList(Engine*);
    Void delRunList(Engine*); 
  
    Int32 getSendSpeed() const;
    Int32 getRecvSpeed() const;

    Uint32 now() const;
    Uint32 monoTick() const; 

    Int32 uploadFile(EvMsgStartUpload* msg);
    Int32 downloadFile(EvMsgStartDownload* msg);    

    inline TaskConfType* getConf() const { return m_data; }

    Void* creatTimer(QueWorker* que, Int32 type); 
    Void startTimer(Void* id, Uint32 tick);
    Void stopTimer(Void* id);    
    Void resetTimer(Void* id, Uint32 tick);
    Void delTimer(Void* id); 

    Void test_01(); 
    Void test_02(); 
  
private: 
    Int32 setup(Void* msg);
    Engine* creatEngine(Int32 cmd, const Char* id);
    Int32 addEngine(Engine* eng);
    Bool exists(const Char* id);
    Bool delEngine(Engine* eng);
    Engine* findEngine(const Char* id);
    
    Void updateSpeed(Bool isSend); 
    
    virtual Void dealEvent(Void* msg); 

    Void doReportResult(const Char* id);

    static Void activeTimer(Void* p1, Void* p2);

    Void doTasks();
    Void freeAllEngine();

private: 
    TickTimer* m_timer;
    I_Comm* m_comm;
    
    TaskConfType* m_data;
    order_list_head m_id_container;
    list_head m_run_container;
};


extern Void* test_upload();
extern Void* test_download();

#endif

