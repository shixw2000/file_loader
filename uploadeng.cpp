#include<string.h>
#include"uploadeng.h"
#include"filereader.h"
#include"cmdctx.h"
#include"uploadcli.h"
#include"taskstat.h"
#include"ictx.h"
#include"msgcenter.h"


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

    case ENUM_TASK_SLOW_STARTUP:
        I_NEW(TaskSlowStartup, pInst);
        break;

    case ENUM_TASK_AVOID_CONG_SENDER:
        I_NEW(TaskAvoidCongest, pInst);
        break;

    case ENUM_TASK_RETRANSMISSION:
        I_NEW(TaskRetrans, pInst);
        break;

    case ENUM_TASK_SEND_FINISH:
        I_NEW(TaskSendFinish, pInst);
        break;

    case ENUM_TASK_SUCCESS_END:
        I_NEW(TaskSucessEnd, pInst);
        break;

    case ENUM_TASK_SEND_TEST:
        I_NEW(TaskSendTest, pInst);
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

SenderEngine::SenderEngine(CmdCtx* env, const Char* id) 
    : Engine(id) {
    setEnv(env);
    setSend(TRUE);
}

SenderEngine::~SenderEngine() {
}

Int32 SenderEngine::start() {
    Int32 ret = 0;
    I_Ctx* ctx = NULL;

    I_NEW_1(Sender, ctx, this);
    setCtx(ctx);

    ret = Engine::start();
    return ret;
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

    m_eng->fail2Peer(errcode);
    
    setState(ENUM_TASK_FAIL_END);
}

Void Sender::prepareSend() { 
    TransBaseType* data = m_eng->getData(); 

    data->m_frame_size = 1;
    data->m_wnd_size = 1;
    data->m_dup_ack_cnt = 0;

    m_reader->prepareSend(data);

    /* start retransmission timeout */
    m_eng->resetWatchdog(DEF_RETRANS_INTERVAL_SEC);
    
    m_eng->startQuarterSec();

    m_eng->startReportTimer();
}

Void Sender::postSend() {
    TransBaseType* data = m_eng->getData(); 

    m_eng->stopWatchdog();
    m_eng->stopQuarterSec();
    m_eng->stopReportTimer();

    m_reader->postSend(data); 
} 

Void Sender::reportResult() {
    postSend();

    m_eng->reportEnd();
}

Void Sender::parseReportStatus(Void* msg) {
    EvMsgReportStatus* pReq = NULL;
    TransBaseType* data = m_eng->getData(); 

    pReq = MsgCenter::cast<EvMsgReportStatus>(msg);

    data->m_progress = pReq->m_progress;
    data->m_realtime_ratio = pReq->m_realtime_ratio; 

    LOG_INFO("parse_report_status| progress=%d| ratio=%d|",
        data->m_progress, data->m_realtime_ratio);
}

