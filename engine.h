#ifndef __ENGINE_H__
#define __ENGINE_H__
/****
    * This file is used as the main task itself.
    * An engine can be an upload client, an upload server,
    * a download client, or a download server.
    * An engine is identified by the task id in cmdctx.
    * Each engine just deals with the right msgs of itself.
    * all of the engines are mapped by cmdctx.
    * an engine startup is from the client manually,
    * or from the acceptance of a server.
****/
#include"globaltype.h"
#include"listnode.h"
#include"queworker.h"
#include"eventmsg.h"
#include"ihandler.h"
#include"filectx.h"


class Engine; 
class CmdCtx;
class I_Ctx;

struct EngineItem {
    list_node m_id_node;
    list_node m_run_node;
    Engine* m_eng;
    Bool m_is_run;
};

class Engine : public QueWorker {
public:
    explicit Engine(const Char* id);
    virtual ~Engine();

    const Char* getID() const; 
    Void setUpload(Bool is_upload);
    Bool isUpload() const;
    Void setSend(Bool is_send);
    Bool isSend() const;
    
    Void setCtx(I_Ctx*); 
    Void setEnv(CmdCtx* env); 

    EngineItem* getItem() {
        return &m_item;
    }

    /* override the startup function for resource preparation */
    virtual Int32 start();

    /* must be called before free */
    virtual Void stop(); 

    virtual Bool notify(Void* msg);

    virtual Bool emerge(Void* msg);

    Uint32 now() const;
    Uint32 monoTick() const;

    TransBaseType* getData() const; 
    
    Int32 sendPkg(Void* msg);
    Int32 transPkg(Void* msg);

    Int32 getConfMss() const;
    Int32 getConfWnd() const;
    Int32 getConfSpeed() const;
    
    Void updateSpeed();

    Void setSpeed(Int32 s);
    Int32 speed() const;

    Int32 quarterSpeed() const;

    Void reportEnd();
    
    Void fillReport(ReportFileInfo* info);

    Void fail2Peer(Int32 error);

    Void startWatchdog(Uint32 tick);
    Void stopWatchdog();
    Void resetWatchdog(Uint32 tick);

    Void startQuarterSec();
    Void stopQuarterSec();

    Void startReportTimer();
    Void stopReportTimer();

    Int32 calcMaxFrame() const;
    Int32 calcMaxWnd() const;

    Void parseReportParam(Void* msg);
    Void sendReportParam();
    
protected:
    Void sendReportParamAck();
    
    Int32 calcConfSpeed() const;
    
    virtual Void dealEvent(Void* msg);

protected:
    CmdCtx* m_env; 
    
private: 
    EngineItem m_item;
    I_Ctx* m_ctx;
    TransBaseType* m_data;
    Void* m_waitdog_timer;
    Void* m_quarter_timer; 
    Void* m_report_timer; 
    Int32 m_conf_speed;
    Bool m_update_speed;
    Bool m_upload;
    Bool m_send;
    Char m_task_id[MAX_TASKID_SIZE];
};

#endif

