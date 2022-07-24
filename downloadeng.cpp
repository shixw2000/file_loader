#include"downloadeng.h"
#include"filewriter.h"
#include"filectx.h"
#include"taskstat.h"
#include"downloadcli.h"
#include"istate.h"
#include"ictx.h"
#include"cmdctx.h"


I_State* ReceiverFactory::creat(Int32 type) {
    I_State* pInst = NULL;
    
    switch (type) {
    case ENUM_TASK_INIT:
        I_NEW(ReceiverInit, pInst);
        break;

    case ENUM_TASK_CONN_DOWNLOAD:
        I_NEW(DownloadCliConn, pInst);
        break;

    case ENUM_TASK_ACCEPT_UPLOAD:
        I_NEW(UploadSrvAccept, pInst);
        break;
    
    case ENUM_TASK_SETUP_RECEIVER:
        I_NEW(TaskSetupReceiver, pInst);
        break;

    case ENUM_TASK_RECV_FINISH:
        I_NEW(TaskRecvFinish, pInst);
        break;

    case ENUM_TASK_SUCCESS_END:
        I_NEW(TaskSucessEnd, pInst);
        break;

    case ENUM_TASK_FAIL_END:
    default:
        I_NEW(TaskFailEnd, pInst);
        break;
    }
    
    return pInst;
}

void ReceiverFactory::release(I_State* pInst) {
    I_FREE(pInst);
}

ReceiverEngine::ReceiverEngine(CmdCtx* env) : Engine(env) {
}

ReceiverEngine::~ReceiverEngine() {
}

Int32 ReceiverEngine::getConfSpeed() const {
    TaskConfType* conf = m_env->getConf();

    return conf->m_max_recv_speed;
}

Void ReceiverEngine::reportResult(const ReportFileInfo*) {
}

I_Ctx* ReceiverEngine::creatCtx() {
    I_Ctx* ctx = NULL;
    
    I_NEW_1(Receiver, ctx, this);
    return ctx;
}

Void ReceiverEngine::freeCtx(I_Ctx* ctx) {
    I_FREE(ctx);
}

Receiver::Receiver(Engine* eng) {
    m_factory = NULL;
    m_writer = NULL;
    m_eng = eng;
}

Receiver::~Receiver() {
}

Int32 Receiver::start() {
    Int32 ret = 0;

    I_NEW(ReceiverFactory, m_factory);
    I_NEW_1(FileWriter, m_writer, m_eng);

    setFactory(m_factory);
    ret = m_writer->start();
    if (0 != ret) {
        return ret;
    }

    ret = I_Ctx::start();
    
    return ret;
}

Void Receiver::stop() {
    I_Ctx::stop();
    
    if (NULL != m_writer) {
        m_writer->stop();
    }
    
    I_FREE(m_writer);
    I_FREE(m_factory);
}

Void Receiver::fail(Int32 errcode) {
    TransBaseType* data = m_eng->getData();

    data->m_last_error = errcode;
    setState(ENUM_TASK_FAIL_END);
}


