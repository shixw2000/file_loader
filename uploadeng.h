#ifndef __UPLOADENG_H__
#define __UPLOADENG_H__
#include"istate.h"
#include"engine.h"
#include"ictx.h"


class CmdCtx;
class FileReader; 

class SenderFactory : public I_Factory{
public: 
    I_State* creat(Int32 type);
    Void release(I_State*);
};

class SenderEngine : public Engine {
public: 
    explicit SenderEngine(CmdCtx* env);
    ~SenderEngine();

    virtual Int32 getConfSpeed() const;
    virtual Void reportResult(const ReportFileInfo* info);

protected:
    virtual I_Ctx* creatCtx();
    virtual Void freeCtx(I_Ctx*);
};

class Sender : public I_Ctx {
public:
    explicit Sender(Engine*);
    ~Sender();

    virtual Int32 start();
    virtual Void stop();

    virtual Void fail(Int32 errcode);

    virtual Engine* engine() const {
        return m_eng;
    }

    FileReader* reader() const {
        return m_reader;
    }
    
private:
    I_Factory* m_factory;
    FileReader* m_reader;
    Engine* m_eng;
};

#endif

