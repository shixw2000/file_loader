#include"uploadeng.h"
#include"filereader.h"
#include"cmdctx.h"
#include"uploadcli.h"
#include"taskstat.h"
#include"filereader.h"
#include"ictx.h"
#include"cmdctx.h"


I_State* SenderFactory::creat(Int32 type) {
    I_State* pInst = NULL;
    
    switch (type) {
    case ENUM_TASK_INIT:
        I_NEW(SenderInit, pInst);
        break;

    case ENUM_TASK_CONN_UPLOAD:
        I_NEW(UploadCliConn, pInst);
        break;

    case ENUM_TASK_ACCEPT_DOWNLOAD:
        I_NEW(DownloadSrvAccept, pInst);
        break;
    
    case ENUM_TASK_SETUP_SENDER:
        I_NEW(TaskSetupSender, pInst);
        break;

    case ENUM_TASK_SEND_FINISH:
        I_NEW(TaskSendFinish, pInst);
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

void SenderFactory::release(I_State* pInst) {
    I_FREE(pInst);
}

SenderEngine::SenderEngine(CmdCtx* env) : Engine(env) {
}

SenderEngine::~SenderEngine() {
}

I_Ctx* SenderEngine::creatCtx() {
    I_Ctx* ctx = NULL;
    
    I_NEW_1(Sender, ctx, this);
    return ctx;
}

Void SenderEngine::freeCtx(I_Ctx* ctx) {
    I_FREE(ctx);
}

Int32 SenderEngine::getConfSpeed() const {
    TaskConfType* conf = m_env->getConf();

    return conf->m_max_send_speed;
}

Void SenderEngine::reportResult(const ReportFileInfo*) {
}


Sender::Sender(Engine* eng) {
    m_factory = NULL;
    m_reader = NULL;
    m_eng = eng;
}

Sender::~Sender() {
}

Int32 Sender::start() {
    Int32 ret = 0;

    I_NEW(SenderFactory, m_factory);
    I_NEW_1(FileReader, m_reader, m_eng);

    setFactory(m_factory);
    ret = m_reader->start();
    if (0 != ret) {
        return ret;
    }

    ret = I_Ctx::start();
    
    return ret;
}

Void Sender::stop() {
    I_Ctx::stop();
    
    if (NULL != m_reader) {
        m_reader->stop();
    }
    
    I_FREE(m_reader);
    I_FREE(m_factory);
}

Void Sender::fail(Int32 errcode) {
    TransBaseType* data = m_eng->getData();

    data->m_last_error = errcode;
    setState(ENUM_TASK_FAIL_END);
}


