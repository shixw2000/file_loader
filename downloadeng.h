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
    ReceiverEngine(CmdCtx* env, const Char* id);
    ~ReceiverEngine(); 
    
    virtual Int32 start();
};


class Receiver : public I_Ctx {
public:
    explicit Receiver(Engine*);
    ~Receiver();

    virtual Int32 start();
    virtual Void stop();

    virtual Void fail(Int32 errcode);

    virtual Engine* engine() const {
        return m_eng;
    }

    FileWriter* writer() const {
        return m_writer;
    }

    Void prepareRecv();
    Void postRecv();

    virtual Void reportResult();

    Void addQuarterRatio(Int32 val);
    Void stepQuarter();
    Int32 averageRatio(); 
    Int32 progress();

    Void sendReportStatus();
    
private:
    I_Factory* m_factory;
    FileWriter* m_writer;
    Engine* m_eng;
    Int32 m_curr_speed;
    Int32 m_pos;
    Int32 m_quarter_ratio[DEF_QUARTER_RATIO_SIZE];
};

#endif

