#include<string.h>
#include"downloadeng.h"
#include"filewriter.h"
#include"filectx.h"
#include"taskstat.h"
#include"downloadcli.h"
#include"istate.h"
#include"ictx.h"
#include"cmdctx.h"
#include"msgcenter.h"


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

ReceiverEngine::ReceiverEngine(CmdCtx* env, const Char* id) 
    : Engine(id) { 
    setEnv(env);
    setSend(FALSE);
}

ReceiverEngine::~ReceiverEngine() {
}

Int32 ReceiverEngine::start() {
    Int32 ret = 0;
    I_Ctx* ctx = NULL;

    I_NEW_1(Receiver, ctx, this);
    setCtx(ctx);

    ret = Engine::start();
    return ret;
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

    m_pos = 0;
    memset(m_quarter_ratio, 0, sizeof(m_quarter_ratio));

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
    m_eng->fail2Peer(errcode);
    
    setState(ENUM_TASK_FAIL_END);
}

Void Receiver::prepareRecv() { 
    TransBaseType* data = m_eng->getData();
    
    m_writer->prepareRecv(data); 
    
    m_eng->resetWatchdog(DEF_BLOCK_TIMEOUT_SEC); 
    m_eng->startQuarterSec();

    m_eng->startReportTimer();
}

Void Receiver::postRecv() {
    TransBaseType* data = m_eng->getData();

    m_eng->stopWatchdog();
    m_eng->stopQuarterSec();
    m_eng->stopReportTimer();

    m_writer->postRecv(data);
}

Void Receiver::reportResult() {
    postRecv();

    m_eng->reportEnd();
}

Void Receiver::addQuarterRatio(Int32 val) {
    m_quarter_ratio[m_pos] += val; 
}

Void Receiver::stepQuarter() {  
    Int32 next = 0;

    next = ((m_pos+1) & DEF_QUARTER_RATIO_MASK);
    
    m_quarter_ratio[next] = 0; 
    m_pos = next;
}

Int32 Receiver::averageRatio() {
    Int32 avg = 0;
    
    for (int i=0; i<DEF_QUARTER_RATIO_SIZE; ++i) {
        if (i != m_pos) {
            avg += m_quarter_ratio[i];
        }
    }

    avg /= DEF_QUARTER_RATIO_MASK;
    return avg;
}

Int32 Receiver::progress() {
    Int64 val = 0;
    TransBaseType* data = m_eng->getData(); 
    
    val = data->m_blk_next_ack;
    val *= 100;
    if (0 < data->m_blk_cnt) {
        val /= data->m_blk_cnt;
    } else {
        val = 100;
    }
    
    return val;
} 

Void Receiver::sendReportStatus() {
    EvMsgReportStatus* pReq = NULL;
    TransBaseType* data = m_eng->getData(); 

    data->m_progress = progress();
    data->m_realtime_ratio = averageRatio();

    pReq = MsgCenter::creatMsg<EvMsgReportStatus>(CMD_EVENT_REPORT_STATUS);
    pReq->m_progress = data->m_progress;
    pReq->m_realtime_ratio = data->m_realtime_ratio; 

    m_eng->sendPkg(pReq);
}

