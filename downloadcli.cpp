#include<string.h>
#include"downloadcli.h"
#include"msgcenter.h"
#include"filetool.h"
#include"filewriter.h"
#include"downloadeng.h"


static const Char DEF_DOWNLOAD_DIR[] = "download"; 

ReceivBase::ReceivBase() {
    m_ctx = NULL;
    m_eng = NULL;
    m_writer = NULL;
}

ReceivBase::~ReceivBase() {
}

Void ReceivBase::prepare(I_Ctx* ctx) { 
    Receiver* receiver = NULL;
    
    receiver = dynamic_cast<Receiver*>(ctx);
    m_ctx = receiver;
    m_writer = receiver->writer();
    m_eng = receiver->engine();

    prepareEx();
}

Void ReceivBase::process(Void* msg) {
    Int32 cmd = 0;
    Int32 ret = 0;
    TransBaseType* data = m_eng->getData();

    cmd = MsgCenter::cmd(msg);
    ret = MsgCenter::error(msg);

    if (CMD_ALARM_TICK_QUARTER_SEC == cmd) {
        m_eng->startQuarterSec();

        dealQuarterTimer();
    } else if (CMD_ALARM_REPORT_TIMEOUT == cmd) {
        m_eng->startReportTimer();

        dealReportTimer();
    } else if (CMD_ERROR_PEER_FAIL != cmd) {
        processEx(msg);
    } else {
        LOG_INFO("receiver_process| cmd=%d| ret=%d| msg=fail|", cmd, ret);

        /* if remote fail, then go to fail directly without a msg sended */
        data->m_last_error = ret; 
        m_ctx->setState(ENUM_TASK_FAIL_END); 
        
        MsgCenter::freeMsg(msg);
    }
}


Void ReceiverInit::processEx(Void* msg) {
    Int32 cmd = 0;
    EvMsgStartDownload* pReq1 = NULL;
    EvMsgReqUpload* pReq2 = NULL;

    cmd = MsgCenter::cmd(msg);
    
    if (CMD_START_FILE_DOWNLOAD == cmd) { 
        pReq1 = MsgCenter::cast<EvMsgStartDownload>(msg);
        
        parseDownload(pReq1);
        m_ctx->setState(ENUM_TASK_CONN_DOWNLOAD);
    } else if (CMD_REQ_FILE_UPLOAD == cmd) { 
        pReq2 = MsgCenter::cast<EvMsgReqUpload>(msg);
        
        parseUpload(pReq2); 
        m_ctx->setState(ENUM_TASK_ACCEPT_UPLOAD);
    
    } else {
        m_ctx->fail(ENUM_ERR_CMD_UNKNOWN);
    }

    MsgCenter::freeMsg(msg);
}

Void ReceiverInit::parseDownload(const EvMsgStartDownload* info) { 
    TransBaseType* data = m_eng->getData();

    m_eng->setUpload(FALSE);

    strncpy(data->m_file_id, info->m_file_id, MAX_FILEID_SIZE);

    strncpy(data->m_file_path, DEF_DOWNLOAD_DIR, MAX_FILEPATH_SIZE);
}

Void ReceiverInit::parseUpload(const EvMsgReqUpload* info) { 
    TransBaseType* data = m_eng->getData();

    m_eng->setUpload(TRUE);
    
    data->m_file_size = info->m_file_size;
    data->m_file_crc = info->m_file_crc;
    data->m_random = info->m_random;
    
    strncpy(data->m_file_name, info->m_file_name, MAX_FILENAME_SIZE); 
    strncpy(data->m_file_path, DEF_DOWNLOAD_DIR, MAX_FILEPATH_SIZE); 

    buildFileID(MsgCenter::ID(info), data);
}

Void DownloadCliConn::prepareEx() {
    Int32 ret = 0;
    EvMsgReqDownload* pReq = NULL;
    TransBaseType* data = m_eng->getData();

    do { 
        data->m_random = sys_random();
        
        /* send a req download msg */
        pReq = buildReq();
  
        ret = m_eng->sendPkg(pReq);
        if (0 != ret) {
            break;
        }

        m_eng->startWatchdog(DEF_CMD_TIMEOUT_SEC); 

        return;
    } while (0);

    /* if deal fail, then change to end */
    m_ctx->fail(ret);
    return;
}

Void DownloadCliConn::processEx(Void* msg) { 
    Int32 cmd = 0;
    EvMsgAckDownload* pRsp = NULL;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_ACK_FILE_DOWNLOAD == cmd) {
        pRsp = MsgCenter::cast<EvMsgAckDownload>(msg);
        
        dealDownloadAck(pRsp);
    } else if (CMD_ALARM_WATCHDOG_TIMEOUT == cmd) {
        LOG_INFO("DownloadCliConn::process| msg=timeout|");

        m_ctx->fail(ENUM_ERR_TIMEOUT); 
    }

    MsgCenter::freeMsg(msg);
}

Void DownloadCliConn::post() {
    m_eng->stopWatchdog(); 
}

EvMsgReqDownload* DownloadCliConn::buildReq() {
    EvMsgReqDownload* info = NULL; 
    TransBaseType* data = m_eng->getData();

    info = MsgCenter::creatMsg<EvMsgReqDownload>(CMD_REQ_FILE_DOWNLOAD);
    
    info->m_random = ++data->m_random;
    strncpy(info->m_file_id, data->m_file_id, MAX_FILEID_SIZE);

    return info;
}

Void DownloadCliConn::parseAck(const EvMsgAckDownload* info) { 
    Int32 localSpeed = 0;
    TransBaseType* data = m_eng->getData();

    data->m_file_size = info->m_file_size;
    data->m_file_crc = info->m_file_crc;
    
    strncpy(data->m_file_name, info->m_file_name, MAX_FILENAME_SIZE); 

    localSpeed = m_eng->getConfSpeed();
    if (localSpeed < info->m_max_speed) {
        m_eng->setSpeed(localSpeed);
    } else {
        m_eng->setSpeed(info->m_max_speed);
    } 
}

EvMsgAckParam* DownloadCliConn::buildAckParam() {
    EvMsgAckParam* info = NULL; 
    TransBaseType* data = m_eng->getData();

    info = MsgCenter::creatMsg<EvMsgAckParam>(CMD_ACK_PARAM);
    
    info->m_max_speed = m_eng->getConfSpeed();
    info->m_blk_beg = data->m_blk_beg;
    info->m_blk_end = data->m_blk_end;

    /* here not increment */
    info->m_random = data->m_random;

    return info;
}

Void DownloadCliConn::dealDownloadAck(EvMsgAckDownload* pRsp) { 
    Int32 ret = 0;
    EvMsgAckParam* pMsg = NULL;
    TransBaseType* data = m_eng->getData();

    do {        
        /* ignoer unmatched ack */
        if (pRsp->m_random != data->m_random) {
            ret = -1;
            break;
        }

        parseAck(pRsp); 
        
        ret = m_writer->prepareFileMap(data);
        if (0 != ret) {
            break;
        } 

        pMsg = buildAckParam();

        ret = m_eng->sendPkg(pMsg);
        if (0 != ret) {
            break;
        }

        m_ctx->setState(ENUM_TASK_SETUP_RECEIVER);
        return;
    } while (0);

    m_ctx->fail(ret);
}


Void UploadSrvAccept::prepareEx() {
    Int32 ret = 0;
    EvMsgAckUpload* pMsg = NULL;
    TransBaseType* data = m_eng->getData();
    
    /*  check mac */ 

    do { 
        ret = m_writer->prepareFileMap(data);
        if (0 != ret) {
            break;
        } 

        pMsg = buildAck();
        
        ret = m_eng->sendPkg(pMsg);
        if (0 != ret) {
            break;
        }

        m_eng->startWatchdog(DEF_CMD_TIMEOUT_SEC);

        return;
    } while (0);

    m_ctx->fail(ret);
    return;
}

Void UploadSrvAccept::processEx(Void* msg) {
    Int32 cmd = 0;
    EvMsgExchParam* pReq = NULL;

    cmd = MsgCenter::cmd(msg); 

    /* nothing in this step */
    LOG_INFO("===========TaskAcceptUpload::process==");

    if (CMD_EXCH_PARAM == cmd) {
        pReq = MsgCenter::cast<EvMsgExchParam>(msg);
        
        dealExchParam(pReq);
    } else if (CMD_ALARM_WATCHDOG_TIMEOUT == cmd) {
        LOG_INFO("UploadSrvAccept::process| msg=timeout|");

        m_ctx->fail(ENUM_ERR_TIMEOUT); 
    }

    MsgCenter::freeMsg(msg);
}

Void UploadSrvAccept::post() {
    m_eng->stopWatchdog();
}

EvMsgAckUpload* UploadSrvAccept::buildAck() {
    EvMsgAckUpload* info = NULL;
    TransBaseType* data = m_eng->getData();

    info = MsgCenter::creatMsg<EvMsgAckUpload>(CMD_ACK_FILE_UPLOAD);
    
    info->m_random = data->m_random;

    return info;
}

Void UploadSrvAccept::dealExchParam(EvMsgExchParam* pReq) {
    Int32 ret = 0;
    EvMsgAckParam* pMsg = NULL;

    do {
        parseParam(pReq);

        pMsg = buildAckParam();
        
        ret = m_eng->sendPkg(pMsg);
        if (0 != ret) {
            break;
        }

        m_ctx->setState(ENUM_TASK_SETUP_RECEIVER);
        return;
    } while (0);

    m_ctx->fail(ret);
    return;
}

Void UploadSrvAccept::parseParam(const EvMsgExchParam* info) { 
    Int32 localSpeed = 0;
    TransBaseType* data = m_eng->getData();

    data->m_random = info->m_random;

    localSpeed = m_eng->getConfSpeed();
    if (localSpeed < info->m_max_speed) {
        m_eng->setSpeed(localSpeed);
    } else {
        m_eng->setSpeed(info->m_max_speed);
    }
}

EvMsgAckParam* UploadSrvAccept::buildAckParam() {
    EvMsgAckParam* info = NULL; 
    TransBaseType* data = m_eng->getData();

    info = MsgCenter::creatMsg<EvMsgAckParam>(CMD_ACK_PARAM);
    
    info->m_max_speed = m_eng->getConfSpeed();
    info->m_blk_beg = data->m_blk_beg;
    info->m_blk_end = data->m_blk_end;

    /* here not increment */
    info->m_random = data->m_random;

    return info;
}


Void TaskSetupReceiver::prepareEx() {
    m_ctx->prepareRecv(); 
}

Void TaskSetupReceiver::processEx(Void* msg) {
    Int32 cmd = 0;
    EvMsgTransData* pReq1 = NULL;
    EvMsgTransBlkFinish* pReq2 = NULL;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_TRANS_DATA_BLK == cmd) {
        pReq1 = MsgCenter::cast<EvMsgTransData>(msg); 

        recvBlkData(pReq1);

        /* no release msg here */
        return;
    } else if (CMD_SEND_BLK_FINISH == cmd) { 
        pReq2 = MsgCenter::cast<EvMsgTransBlkFinish>(msg);

        dealFinish(pReq2); 
    } else if (CMD_EVENT_REPORT_PARAM == cmd
        || CMD_EVENT_REPORT_PARAM_ACK == cmd) {
        m_eng->parseReportParam(msg);
    } else if (CMD_ALARM_TRANS_BLK == cmd) {
        m_ctx->fail(ENUM_ERR_WRITE_BLK);
    } else if (CMD_ALARM_WATCHDOG_TIMEOUT == cmd) {
        LOG_INFO("TaskSetupReceiver::process| msg=timeout|");

        m_ctx->fail(ENUM_ERR_TIMEOUT); 
    }

    MsgCenter::freeMsg(msg);
}

Void TaskSetupReceiver::recvBlkData(EvMsgTransData* pReq) {
    Int32 ret = 0;
    Bool bOk = TRUE;
    Int32 size = 0;
    TransBaseType* data = m_eng->getData(); 

    bOk = FileTool::chkBlkValid(pReq->m_blk_beg, pReq->m_blk_end,
        data->m_blk_beg, data->m_blk_end);
    if (!bOk) {
        MsgCenter::freeMsg(pReq);
        
        /* invalid blk, ignore */
        return;
    }

    size = FileTool::calcBlkSize(pReq->m_blk_beg,
        pReq->m_blk_end, data->m_file_size);
    if (size != pReq->m_buf_size) { 
        MsgCenter::freeMsg(pReq);
        
        /* invalid blk, ignore */
        return;
    }

#ifdef __TEST_LOST_PKG__
    {
        /* for test timeout and pkg lost */
        Uint32 n = randTick();
        if (!(n & 0xf)) {
            LOG_INFO(" ===lose pkg| beg=%d| end=%d| time=%u|",
                pReq->m_blk_beg, pReq->m_blk_end, pReq->m_time);
            
            MsgCenter::freeMsg(pReq);
            return;
        }
    }
#endif

    m_ctx->addQuarterRatio(pReq->m_blk_end - pReq->m_blk_beg);

    /* reset the timer */
    m_eng->resetWatchdog(DEF_BLOCK_TIMEOUT_SEC);

    ret = m_writer->notifyRecv(pReq, data);
    if (0 != ret) {
        m_ctx->fail(-1);
    }
}

Void TaskSetupReceiver::dealQuarterTimer() { 
    m_ctx->stepQuarter(); 
}

Void TaskSetupReceiver::dealReportTimer() {
    m_eng->sendReportParam();

    m_ctx->sendReportStatus();
}

Void TaskSetupReceiver::dealFinish(EvMsgTransBlkFinish* pReq) {
    Int32 ret = 0;
    TransBaseType* data = m_eng->getData(); 
  
    if (pReq->m_blk_beg == data->m_blk_beg
        && pReq->m_blk_end == data->m_blk_end) {
        
        ret = m_writer->dealBlkFinish(data);
        if (0 == ret) {
            m_ctx->setState(ENUM_TASK_RECV_FINISH);
        } else {
            m_ctx->fail(-1);
        } 
    }
}


Void TaskRecvFinish::prepareEx() { 
    m_eng->resetWatchdog(DEF_CMD_TIMEOUT_SEC); 
    
    notifyFinish(); 
}

Void TaskRecvFinish::processEx(Void* msg) {
    Int32 cmd = 0;
    EvMsgTaskCompleted* pRsp = NULL;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_TASK_COMPLETED == cmd) {
        pRsp = MsgCenter::cast<EvMsgTaskCompleted>(msg);
  
        dealFinish(pRsp);
    } else if (CMD_EVENT_REPORT_PARAM == cmd
        || CMD_EVENT_REPORT_PARAM_ACK == cmd) {
        m_eng->parseReportParam(msg); 
    } else if (CMD_ALARM_WATCHDOG_TIMEOUT == cmd) {
        LOG_INFO("TaskRecvFinish::process| msg=timeout|");

        m_ctx->fail(ENUM_ERR_TIMEOUT); 
    }
    
    MsgCenter::freeMsg(msg);
} 

Void TaskRecvFinish::notifyFinish() {
    Int32 ret = 0;
    EvMsgTaskCompleted* info = NULL;
    TransBaseType* data = m_eng->getData(); 

    info = MsgCenter::creatMsg<EvMsgTaskCompleted>(CMD_TASK_COMPLETED);
    strncpy(info->m_file_id, data->m_file_id, MAX_FILEID_SIZE); 
    
    ret = m_eng->sendPkg(info);
    if (0 != ret) {
        m_ctx->fail(ret);
    } 
}

Void TaskRecvFinish::dealQuarterTimer() {
    m_ctx->stepQuarter();
}

Void TaskRecvFinish::dealFinish(EvMsgTaskCompleted* pRsp) {
    TransBaseType* data = m_eng->getData(); 
        
    if (0 == strncmp(data->m_file_id, pRsp->m_file_id, MAX_FILEID_SIZE)) {
        m_ctx->setState(ENUM_TASK_SUCCESS_END);
    } else {
        m_ctx->fail(-1);
    } 
} 

Void TaskRecvFinish::post() {
    m_eng->stopWatchdog();
}


