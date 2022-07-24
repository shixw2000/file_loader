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
#include"queworker.h"
#include"eventmsg.h"
#include"filectx.h"


enum EnumEngStatus {
    ENUM_ENG_IDLE = 0,
    ENUM_ENG_RUNNING,
};

class CmdCtx;
class I_Ctx;

class Engine : public QueWorker {
public:
    explicit Engine(CmdCtx* env);
    virtual ~Engine();

    /* override the startup function for resource preparation */
    virtual Int32 start();

    /* must be called before free */
    virtual Void stop();

    Void setID(const Char id[]);
    const Char* getID() const; 

    Int32 status() const {
        return m_status;
    }

    void setStatus(Int32 status) {
        m_status = status;
    }

    virtual Bool notify(Void* msg);

    virtual Bool emerge(Void* msg);

    Uint32 now() const;

    TransBaseType* getData() const; 
    
    Int32 sendPkg(Void* msg);
    Int32 transPkg(Void* msg);

    Void* creatTimer(Int32 type); 
    Void delTimer(Void* id);

    Void resetTimer(Void* id, Uint32 tick);
    Void startTimer(Void* id, Uint32 tick);
    Void stopTimer(Void* id); 

    bool operator<(const Engine* o) const;
    bool operator==(const Engine* o) const;

    virtual Int32 getConfSpeed() const = 0; 

protected:
    virtual Void dealEvent(Void* msg);
    virtual I_Ctx* creatCtx() = 0;
    virtual Void freeCtx(I_Ctx*) = 0;

protected:
    CmdCtx* m_env;
    
private: 
    I_Ctx* m_ctx;
    TransBaseType* m_data;
    Int32 m_status;
};

#endif

