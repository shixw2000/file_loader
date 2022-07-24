#include<string.h>
#include"downloadcli.h"
#include"msgcenter.h"
#include"filetool.h"
#include"filewriter.h"
#include"downloadeng.h"


static const Char DEF_DOWNLOAD_DIR[] = "download"; 

ReceiverInit::ReceiverInit() {
    m_ctx = NULL;
    m_eng = NULL;
    m_writer = NULL;
}

ReceiverInit::~ReceiverInit() {
}

Void ReceiverInit::prepare(I_Ctx* ctx) { 
    Receiver* receiver = NULL;
    
    receiver = dynamic_cast<Receiver*>(ctx);
    m_ctx = ctx;
    m_writer = receiver->writer();
    m_eng = receiver->engine();
}

Void ReceiverInit::process(Void* msg) {
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

    data->m_upload_download = ENUM_TASK_TYPE_DOWNLOAD;
    data->m_send_recv = ENUM_DATA_TYPE_RECV;

    strncpy(data->m_file_id, info->m_file_id, MAX_FILEID_SIZE);

    strncpy(data->m_file_path, DEF_DOWNLOAD_DIR, MAX_FILEPATH_SIZE);
}

Void ReceiverInit::parseUpload(const EvMsgReqUpload* info) { 
    TransBaseType* data = m_eng->getData();

    data->m_upload_download = ENUM_TASK_TYPE_UPLOAD;
    data->m_send_recv = ENUM_DATA_TYPE_RECV;
    
    data->m_file_size = info->m_file_size;
    data->m_file_crc = info->m_file_crc;
    data->m_random = info->m_random;
    
    strncpy(data->m_file_name, info->m_file_name, MAX_FILENAME_SIZE); 
    strncpy(data->m_file_path, DEF_DOWNLOAD_DIR, MAX_FILEPATH_SIZE); 

    buildFileID(MsgCenter::ID(info), data);
}

DownloadCliConn::DownloadCliConn() {
    m_ctx = NULL;
    m_eng = NULL;
    m_writer = NULL;
    m_timerID = NULL;
}

DownloadCliConn::~DownloadCliConn() {
}

Void DownloadCliConn::prepare(I_Ctx* ctx) {
    Int32 ret = 0;
    EvMsgReqDownload* pReq = NULL;
    TransBaseType* data = NULL;
    Receiver* receiver = NULL; 

    do { 
        receiver = dynamic_cast<Receiver*>(ctx);
        m_ctx = ctx;
        m_writer = receiver->writer();
        m_eng = receiver->engine();
        
        data = m_eng->getData();
        
        data->m_random = MsgCenter::random();
        
        /* send a req download msg */
        pReq = buildReq();
  
        ret = m_eng->sendPkg(pReq);
        if (0 != ret) {
            break;
        }

        /* wait timeout */
        m_timerID = m_eng->creatTimer(CMD_ALARM_TICK_TIMEOUT);
        if (NULL != m_timerID) {
            m_eng->startTimer(m_timerID, DEF_CMD_TIMEOUT_SEC);
        }

        return;
    } while (0);

    /* if deal fail, then change to end */
    m_ctx->fail(ret);
    return;
}

Void DownloadCliConn::process(Void* msg) { 
    Int32 cmd = 0;
    EvMsgAckDownload* pRsp = NULL;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_ACK_FILE_DOWNLOAD == cmd) {
        pRsp = MsgCenter::cast<EvMsgAckDownload>(msg);
        
        dealDownloadAck(pRsp);
    } else if (CMD_ALARM_TICK_TIMEOUT == cmd) {
        /* timeout */
        LOG_INFO("=============download timerout");

        m_ctx->fail(ENUM_ERR_TIMEOUT);
    } else {
        /* unknown cmd */
    }

    MsgCenter::freeMsg(msg);
}

Void DownloadCliConn::post() {
    if (NULL != m_timerID) {
        m_eng->delTimer(m_timerID);

        m_timerID = NULL;
    }
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
        data->m_max_speed = localSpeed;
    } else {
        data->m_max_speed = info->m_max_speed;
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

        ret = MsgCenter::error(pRsp);
        if (0 != ret) {
            ret = -2;
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


UploadSrvAccept::UploadSrvAccept() {
    m_ctx = NULL;
    m_eng = NULL;
    m_writer = NULL;
    m_timerID = NULL;
}

UploadSrvAccept::~UploadSrvAccept() {
}

Void UploadSrvAccept::prepare(I_Ctx* ctx) {
    Int32 ret = 0;
    EvMsgAckUpload* pMsg = NULL;
    TransBaseType* data = NULL;
    Receiver* receiver = NULL; 
    
    /*  check mac */ 

    do { 
        receiver = dynamic_cast<Receiver*>(ctx);
        m_ctx = ctx;
        m_writer = receiver->writer();
        m_eng = receiver->engine();
        
        data = m_eng->getData();
        
        ret = m_writer->prepareFileMap(data);
        if (0 != ret) {
            break;
        } 

        pMsg = buildAck();
        
        ret = m_eng->sendPkg(pMsg);
        if (0 != ret) {
            break;
        }

        /* wait timeout */
        m_timerID = m_eng->creatTimer(CMD_ALARM_TICK_TIMEOUT);
        if (NULL != m_timerID) {
            m_eng->startTimer(m_timerID, DEF_CMD_TIMEOUT_SEC);
        }

        return;
    } while (0);

    m_ctx->fail(ret);
    return;
}

Void UploadSrvAccept::process(Void* msg) {
    Int32 cmd = 0;
    EvMsgExchParam* pReq = NULL;

    cmd = MsgCenter::cmd(msg); 

    /* nothing in this step */
    LOG_INFO("===========TaskAcceptUpload::process==");

    if (CMD_EXCH_PARAM == cmd) {
        pReq = MsgCenter::cast<EvMsgExchParam>(msg);
        
        dealExchParam(pReq);
    } else if (CMD_ALARM_TICK_TIMEOUT == cmd) {
        /* timeout */
        LOG_INFO("=============TaskAcceptUpload timerout");

        m_ctx->fail(ENUM_ERR_TIMEOUT);
    }

    MsgCenter::freeMsg(msg);
}

Void UploadSrvAccept::post() {
    if (NULL != m_timerID) {
        m_eng->delTimer(m_timerID);

        m_timerID = NULL;
    }
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
        data->m_max_speed = localSpeed;
    } else {
        data->m_max_speed = info->m_max_speed;
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


TaskSetupReceiver::TaskSetupReceiver() {
    m_ctx = NULL;
    m_eng = NULL;
    m_writer = NULL;
    m_timerID = NULL;
}

TaskSetupReceiver::~TaskSetupReceiver() {
}

Void TaskSetupReceiver::prepare(I_Ctx* ctx) {
    TransBaseType* data = NULL;
    Receiver* receiver = NULL;
    
    receiver = dynamic_cast<Receiver*>(ctx);
    m_ctx = ctx;
    m_writer = receiver->writer();
    m_eng = receiver->engine();

    data = m_eng->getData();
    
    m_writer->prepareRecv(data);

    /* wait timeout */
    m_timerID = m_eng->creatTimer(CMD_ALARM_TICK_TIMEOUT);
    if (NULL != m_timerID) {
        m_eng->startTimer(m_timerID, DEF_BLOCK_TIMEOUT_SEC);
    } 
}

Void TaskSetupReceiver::process(Void* msg) {
    Int32 cmd = 0;
    EvMsgTransData* pReq1 = NULL;
    EvMsgTransBlkFinish* pReq2 = NULL;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_TRANS_DATA_BLK == cmd) {
        pReq1 = MsgCenter::cast<EvMsgTransData>(msg);

        /* reset the timer */
        m_eng->resetTimer(m_timerID, DEF_BLOCK_TIMEOUT_SEC);

        recvBlkData(pReq1);

        /* no release msg here */
        return;
    } else if (CMD_SEND_BLK_FINISH == cmd) { 
        pReq2 = MsgCenter::cast<EvMsgTransBlkFinish>(msg);

        m_eng->stopTimer(m_timerID);
        
        dealFinish(pReq2); 
    } else if (CMD_ALARM_TRANS_BLK == cmd) {
        m_ctx->fail(ENUM_ERR_WRITE_BLK);
    } else if (CMD_ALARM_TICK_TIMEOUT == cmd) {
        /* timeout */
        LOG_INFO("=============TaskSetupReceiver timerout");

        m_ctx->fail(ENUM_ERR_TIMEOUT);
    } else {
    }

    MsgCenter::freeMsg(msg);
}

Void TaskSetupReceiver::post() { 
    if (NULL != m_timerID) {
        m_eng->delTimer(m_timerID);

        m_timerID = NULL;
    }
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

    ret = m_writer->notifyRecv(pReq, data);
    if (0 != ret) {
        m_ctx->fail(-1);
    }
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


TaskRecvFinish::TaskRecvFinish() {
    m_ctx = NULL;
    m_eng = NULL;
    m_writer = NULL;
    m_timerID = NULL; 
}

TaskRecvFinish::~TaskRecvFinish() {
}

Void TaskRecvFinish::prepare(I_Ctx* ctx) {
    Receiver* receiver = NULL;
    
    receiver = dynamic_cast<Receiver*>(ctx);
    m_ctx = ctx;
    m_writer = receiver->writer();
    m_eng = receiver->engine();
    
    notifyFinish();
        
    /* wait timeout */
    m_timerID = m_eng->creatTimer(CMD_ALARM_TICK_TIMEOUT);
    if (NULL != m_timerID) {
        m_eng->startTimer(m_timerID, DEF_BLOCK_TIMEOUT_SEC);
    }
}

Void TaskRecvFinish::process(Void* msg) {
    Int32 cmd = 0;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_ALARM_TICK_TIMEOUT == cmd) {
        /* timeout */
        LOG_INFO("=============TaskSetupSender timerout");

        m_ctx->fail(ENUM_ERR_TIMEOUT);
    } else {
    }

    MsgCenter::freeMsg(msg);
} 

Void TaskRecvFinish::post() { 
    if (NULL != m_timerID) {
        m_eng->delTimer(m_timerID);

        m_timerID = NULL;
    }
}

Void TaskRecvFinish::notifyFinish() {
    Int32 ret = 0;
    EvMsgTransAckFinish* info = NULL;
    TransBaseType* data = m_eng->getData(); 

    info = MsgCenter::creatMsg<EvMsgTransAckFinish>(CMD_RECV_BLK_FINISH);
    info->m_blk_beg = data->m_blk_beg;
    info->m_blk_end = data->m_blk_end;
    strncpy(info->m_file_id, data->m_file_id, MAX_FILEID_SIZE); 
    
    ret = m_eng->sendPkg(info);
    if (0 != ret) {
        m_ctx->fail(ret);
    } 
}

