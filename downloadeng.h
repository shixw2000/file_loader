#ifndef __DOWNLOADENG_H__
#define __DOWNLOADENG_H__
#include"globaltype.h"
#include"engine.h"
#include"filectx.h"
#include"istate.h"
#include"ictx.h"


class CmdCtx;
class FileWriter;

class ReceiverFactory : public I_Factory{
public: 
    I_State* creat(Int32 type);
    Void release(I_State*);
};

class ReceiverEngine : public Engine {
public:
    explicit ReceiverEngine(CmdCtx* env);
    ~ReceiverEngine(); 

    virtual Int32 getConfSpeed() const;
    
    virtual Void reportResult(const ReportFileInfo* info);

protected:
    virtual I_Ctx* creatCtx();
    virtual Void freeCtx(I_Ctx*);
};


class Receiver : public I_Ctx {
public:
    explicit Receiver(Engine*);
    ~Receiver();

    virtual Int32 start();
    virtual Void stop();

    virtual Void fail(Int32 errcode);

    Engine* engine() const {
        return m_eng;
    }

    FileWriter* writer() const {
        return m_writer;
    }
    
private:
    I_Factory* m_factory;
    FileWriter* m_writer;
    Engine* m_eng;
};

#endif

