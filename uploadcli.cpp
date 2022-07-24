#include<string.h>
#include"uploadcli.h"
#include"msgcenter.h" 
#include"filetool.h"
#include"filereader.h"
#include"uploadeng.h"


static const Char DEF_UPLOAD_DIR[] = "upload";


SenderInit::SenderInit() {
    m_ctx = NULL;
    m_eng = NULL;
    m_reader = NULL;
}

SenderInit::~SenderInit() {
}

Void SenderInit::prepare(I_Ctx* ctx) { 
    Sender* sender = NULL;
    
    sender = dynamic_cast<Sender*>(ctx);
    m_ctx = ctx;
    m_reader = sender->reader();
    m_eng = sender->engine();
}

Void SenderInit::process(Void* msg) {
    Int32 cmd = 0;
    EvMsgStartUpload* pReq1 = NULL;
    EvMsgReqDownload* pReq2 = NULL;

    cmd = MsgCenter::cmd(msg);
    
    if (CMD_START_FILE_UPLOAD == cmd) { 
        pReq1 = MsgCenter::cast<EvMsgStartUpload>(msg);
        
        parseUpload(pReq1);
        m_ctx->setState(ENUM_TASK_CONN_UPLOAD);
    } else if (CMD_REQ_FILE_DOWNLOAD == cmd) { 
        pReq2 = MsgCenter::cast<EvMsgReqDownload>(msg);
        
        parseDownload(pReq2);
        m_ctx->setState(ENUM_TASK_ACCEPT_DOWNLOAD);
    } else {
        m_ctx->fail(ENUM_ERR_CMD_UNKNOWN);
    }

    MsgCenter::freeMsg(msg);
}

Void SenderInit::parseUpload(const EvMsgStartUpload* info) {
    TransBaseType* data = m_eng->getData();

    data->m_upload_download = ENUM_TASK_TYPE_UPLOAD;
    data->m_send_recv = ENUM_DATA_TYPE_SEND;
    
    data->m_file_size = info->m_file_size;
    data->m_file_crc = info->m_file_crc;
    
    strncpy(data->m_file_name, info->m_file_name, MAX_FILENAME_SIZE);
    strncpy(data->m_file_path, DEF_UPLOAD_DIR, MAX_FILEPATH_SIZE);
}

Void SenderInit::parseDownload(const EvMsgReqDownload* info) {
    TransBaseType* data = m_eng->getData();

    data->m_upload_download = ENUM_TASK_TYPE_DOWNLOAD;
    data->m_send_recv = ENUM_DATA_TYPE_SEND;

    data->m_random = info->m_random;
    strncpy(data->m_file_id, info->m_file_id, MAX_FILEID_SIZE);
    strncpy(data->m_file_path, DEF_UPLOAD_DIR, MAX_FILEPATH_SIZE);
}

UploadCliConn::UploadCliConn() {
    m_ctx = NULL;
    m_eng = NULL;
    m_reader = NULL;
    m_timerID = NULL;
}

UploadCliConn::~UploadCliConn() {
}

Void UploadCliConn::prepare(I_Ctx* ctx) {
    Int32 ret = 0;
    EvMsgReqUpload* pReq = NULL;
    TransBaseType* data = NULL;
    Sender* sender = NULL; 
    
    LOG_INFO("===========TaskConnUpload::prepare==");

    do { 
        sender = dynamic_cast<Sender*>(ctx);
        m_ctx = ctx;
        m_reader = sender->reader();
        m_eng = sender->engine();
    
        data = m_eng->getData();
        
        data->m_random = MsgCenter::random(); 
        
        /* check file params*/
        ret = m_reader->parseRawFile(data);
        if (0 != ret) {
            break;
        }

        /* send a req upload msg */
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

    m_ctx->fail(ret);
    return;
}

Void UploadCliConn::process(Void* msg) { 
    Int32 cmd = 0;
    EvMsgAckUpload* pAckUpload = NULL;
    EvMsgAckParam* pAckParam = NULL;

    cmd = MsgCenter::cmd(msg);
    
    if (CMD_ACK_FILE_UPLOAD == cmd) {
        pAckUpload = MsgCenter::cast<EvMsgAckUpload>(msg);
        
        dealUploadAck(pAckUpload);
    } else if (CMD_ACK_PARAM == cmd) {
        pAckParam = MsgCenter::cast<EvMsgAckParam>(msg);
        
        dealParamAck(pAckParam);
    } else if (CMD_ALARM_TICK_TIMEOUT == cmd) {
        LOG_INFO("=============upload timerout");

        m_ctx->fail(ENUM_ERR_TIMEOUT);
    } else {
        /* unknown cmd */
    }

    MsgCenter::freeMsg(msg);
}

Void UploadCliConn::post() {
    if (NULL != m_timerID) {
        m_eng->delTimer(m_timerID);

        m_timerID = NULL;
    }
}

EvMsgReqUpload* UploadCliConn::buildReq() {
    EvMsgReqUpload* info = NULL;
    TransBaseType* data = m_eng->getData();
    
    info = MsgCenter::creatMsg<EvMsgReqUpload>(CMD_REQ_FILE_UPLOAD);
    
    info->m_file_size = data->m_file_size;
    info->m_file_crc = data->m_file_crc;
    info->m_random = ++data->m_random;
        
    strncpy(info->m_file_name, data->m_file_name, MAX_FILENAME_SIZE);
    return info;
}

Void UploadCliConn::dealUploadAck(EvMsgAckUpload* pRsp) {
    Int32 ret = 0;
    EvMsgExchParam* pMsg = NULL;
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

        pMsg = buildExchParam();

        ret = m_eng->sendPkg(pMsg);
        if (0 != ret) {
            ret = -3;
            break;
        }

        return;
    } while (0);

    m_ctx->fail(ret);
}

EvMsgExchParam* UploadCliConn::buildExchParam() {
    EvMsgExchParam* info = NULL; 
    TransBaseType* data = m_eng->getData();
    
    info = MsgCenter::creatMsg<EvMsgExchParam>(CMD_EXCH_PARAM);

    info->m_random = ++data->m_random;
    info->m_max_speed = m_eng->getConfSpeed(); 
    return info;
}

Void UploadCliConn::dealParamAck(EvMsgAckParam* pRsp) {
    Int32 ret = 0;
    TransBaseType* data = m_eng->getData();

    do {
        /* ignoer unmatched ack */
        if (pRsp->m_random != data->m_random) {
            ret = -1;
            break;
        }

        if (0 <= pRsp->m_blk_beg && pRsp->m_blk_beg <= pRsp->m_blk_end
            && pRsp->m_blk_end <= data->m_blk_cnt) {
            parseParamAck(pRsp);
        } else {
            ret = -2;
            break;
        } 
        
        m_ctx->setState(ENUM_TASK_SETUP_SENDER);
        return;
    } while (0);

    m_ctx->fail(ret);
}

Void UploadCliConn::parseParamAck(const EvMsgAckParam* info) {
    Int32 localSpeed = 0;
    TransBaseType* data = m_eng->getData(); 

    data->m_blk_beg = info->m_blk_beg;
    data->m_blk_end = info->m_blk_end;
    
    data->m_blk_next = data->m_blk_beg;
    data->m_blk_next_ack = data->m_blk_beg;

    localSpeed = m_eng->getConfSpeed();
    if (localSpeed < info->m_max_speed) {
        data->m_max_speed = localSpeed;
    } else {
        data->m_max_speed = info->m_max_speed;
    }
} 

DownloadSrvAccept::DownloadSrvAccept() {
    m_ctx = NULL;
    m_eng = NULL;
    m_reader = NULL;
    m_timerID = NULL;
}

DownloadSrvAccept::~DownloadSrvAccept() {
}

Void DownloadSrvAccept::prepare(I_Ctx* ctx) {
    Int32 ret = 0;
    EvMsgAckDownload* pRsp = NULL;
    TransBaseType* data = NULL;
    Sender* sender = NULL; 

    /*  check mac */

    do {   
        sender = dynamic_cast<Sender*>(ctx);
        m_ctx = ctx;
        m_reader = sender->reader();
        m_eng = sender->engine();
 
        data = m_eng->getData();
    
        ret = m_reader->parseMapFile(data);
        if (0 != ret) {
            ret = -1;
            break;
        }
   
        pRsp = buildAck();
        
        ret = m_eng->sendPkg(pRsp);
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

Void DownloadSrvAccept::process(Void* msg) {
    Int32 cmd = 0;
    EvMsgAckParam* pAckParam = NULL;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_ACK_PARAM == cmd) {
        pAckParam = MsgCenter::cast<EvMsgAckParam>(msg);
        
        dealParamAck(pAckParam); 
    } else if (CMD_ALARM_TICK_TIMEOUT == cmd) {
        /* timeout */
        LOG_INFO("=============TaskAcceptDownload timerout");

        m_ctx->fail(ENUM_ERR_TIMEOUT); 
    } else {
        /* unknown cmd */
    }

    MsgCenter::freeMsg(msg);
}

Void DownloadSrvAccept::post() {
    if (NULL != m_timerID) {
        m_eng->delTimer(m_timerID);

        m_timerID = NULL;
    }
}

EvMsgAckDownload* DownloadSrvAccept::buildAck() {
    EvMsgAckDownload* info = NULL;
    TransBaseType* data = m_eng->getData();
    
    info = MsgCenter::creatMsg<EvMsgAckDownload>(CMD_ACK_FILE_DOWNLOAD);
    
    info->m_file_size = data->m_file_size;
    info->m_file_crc = data->m_file_crc;
    info->m_random = data->m_random;
    
    info->m_max_speed = m_eng->getConfSpeed();
        
    strncpy(info->m_file_name, data->m_file_name, MAX_FILENAME_SIZE);
    return info;
}

Void DownloadSrvAccept::dealParamAck(EvMsgAckParam* pRsp) {
    Int32 ret = 0;
    TransBaseType* data = m_eng->getData();

    do {
        /* ignoer unmatched ack */
        if (pRsp->m_random != data->m_random) {
            ret = -1;
            break;
        }

        if (0 <= pRsp->m_blk_beg && pRsp->m_blk_beg <= pRsp->m_blk_end
            && pRsp->m_blk_end <= data->m_blk_cnt) {
            parseParamAck(pRsp);
        } else {
            ret = -2;
            break;
        } 
        
        m_ctx->setState(ENUM_TASK_SETUP_SENDER);
        return;
    } while (0);

    m_ctx->fail(ret);
}

Void DownloadSrvAccept::parseParamAck(const EvMsgAckParam* info) {
    Int32 localSpeed = 0;
    TransBaseType* data = m_eng->getData(); 

    data->m_blk_beg = info->m_blk_beg;
    data->m_blk_end = info->m_blk_end;
    
    data->m_blk_next = data->m_blk_beg;
    data->m_blk_next_ack = data->m_blk_beg;

    localSpeed = m_eng->getConfSpeed();
    if (localSpeed < info->m_max_speed) {
        data->m_max_speed = localSpeed;
    } else {
        data->m_max_speed = info->m_max_speed;
    }
}

TaskSetupSender::TaskSetupSender() {
    m_ctx = NULL;
    m_eng = NULL;
    m_reader = NULL;
    m_timerID = NULL; 
}

TaskSetupSender::~TaskSetupSender() {
}

Void TaskSetupSender::prepare(I_Ctx* ctx) {
    Sender* sender = NULL;
    
    sender = dynamic_cast<Sender*>(ctx);
    m_ctx = ctx;
    m_reader = sender->reader();
    m_eng = sender->engine();
  
    prepareSend();
    
    /* wait timeout */
    m_timerID = m_eng->creatTimer(CMD_ALARM_TICK_TIMEOUT);
    if (NULL != m_timerID) {
        m_eng->startTimer(m_timerID, DEF_BLOCK_TIMEOUT_SEC);
    } 
}

Void TaskSetupSender::process(Void* msg) {
    Int32 cmd = 0;
    EvMsgTransDataAck* pRsp = NULL;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_TRANS_DATA_BLK_ACK == cmd) {
        pRsp = MsgCenter::cast<EvMsgTransDataAck>(msg);

        /* reset the timer */
        m_eng->resetTimer(m_timerID, DEF_BLOCK_TIMEOUT_SEC);
        
        dealBlkAck(pRsp);
    } else if (CMD_ALARM_TRANS_BLK == cmd) {
        m_ctx->fail(ENUM_ERR_READ_BLK);
    } else if (CMD_ALARM_TICK_TIMEOUT == cmd) {
        /* timeout */
        LOG_INFO("=============TaskSetupSender timerout");

        m_ctx->fail(ENUM_ERR_TIMEOUT);
    } else {
    }

    MsgCenter::freeMsg(msg);
} 

Void TaskSetupSender::post() { 
    if (NULL != m_timerID) {
        m_eng->delTimer(m_timerID);

        m_timerID = NULL;
    }
}

Void TaskSetupSender::sendBlk(TransBaseType* data) {
    Int32 ret = 0;

    ret = m_reader->notifySend(data);
    if (0 == ret) {
        if (data->m_blk_next_ack == data->m_blk_end) {
            m_ctx->setState(ENUM_TASK_SEND_FINISH);
        }
    } else {
        m_ctx->fail(ret);
    }
}

Void TaskSetupSender::dealBlkAck(EvMsgTransDataAck* pRsp) {
    TransBaseType* data = m_eng->getData(); 

    if (data->m_blk_beg <= pRsp->m_blk_next
        && data->m_blk_end >= pRsp->m_blk_next) {
        if (data->m_blk_next_ack < pRsp->m_blk_next) {

            m_reader->purgeWaitList(pRsp->m_blk_next, data); 
            sendBlk(data);
        } else if (data->m_blk_next_ack == pRsp->m_blk_next) {
            /* duplicated ack for 3 times means  a losing pkg */
        } else {
        }
    }
}

Void TaskSetupSender::prepareSend() {
    TransBaseType* data = m_eng->getData(); 

    m_reader->prepareSend(data);
    sendBlk(data);
}


TaskSendFinish::TaskSendFinish() {
    m_ctx = NULL;
    m_eng = NULL;
    m_reader = NULL;
    m_timerID = NULL; 
}

TaskSendFinish::~TaskSendFinish() {
}

Void TaskSendFinish::prepare(I_Ctx* ctx) {
    Int32 ret = 0;
    Sender* sender = NULL; 

    do {     
        sender = dynamic_cast<Sender*>(ctx);
        m_ctx = ctx;
        m_reader = sender->reader();
        m_eng = sender->engine();
        
        ret = notifyFinish();
        if (0 != ret) {
            ret = -1;
            break;
        }
        
        /* wait timeout */
        m_timerID = m_eng->creatTimer(CMD_ALARM_TICK_TIMEOUT);
        if (NULL != m_timerID) {
            m_eng->startTimer(m_timerID, DEF_BLOCK_TIMEOUT_SEC);
        }

        return;
    } while (0);

    m_ctx->fail(ret);
    return;
}

Void TaskSendFinish::process(Void* msg) {
    Int32 cmd = 0;
    EvMsgTransAckFinish* pRsp = NULL;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_RECV_BLK_FINISH == cmd) {
        pRsp = MsgCenter::cast<EvMsgTransAckFinish>(msg);
  
        dealFinish(pRsp);
    } else if (CMD_ALARM_TICK_TIMEOUT == cmd) {
        /* timeout */
        LOG_INFO("=============TaskSetupSender timerout");

        m_ctx->fail(ENUM_ERR_TIMEOUT);
    } else {
    }

    MsgCenter::freeMsg(msg);
} 

Void TaskSendFinish::post() { 
    if (NULL != m_timerID) {
        m_eng->delTimer(m_timerID);

        m_timerID = NULL;
    }
}

Int32 TaskSendFinish::notifyFinish() {
    Int32 ret = 0;
    EvMsgTransBlkFinish* pMsg = NULL; 
    TransBaseType* data = m_eng->getData(); 
    
    pMsg = MsgCenter::creatMsg<EvMsgTransBlkFinish>(CMD_SEND_BLK_FINISH);
    pMsg->m_blk_beg = data->m_blk_beg;
    pMsg->m_blk_end = data->m_blk_end; 
    
    ret = m_eng->sendPkg(pMsg);
    if (0 == ret) {
        return 0;
    } else {
        return -1;
    }
}

Void TaskSendFinish::dealFinish(EvMsgTransAckFinish* pRsp) {
    TransBaseType* data = m_eng->getData(); 
        
    if (pRsp->m_blk_beg == data->m_blk_beg 
        && pRsp->m_blk_end == data->m_blk_end) {
        if (ENUM_TASK_TYPE_UPLOAD == data->m_upload_download) {
            strncpy(data->m_file_id, pRsp->m_file_id, MAX_FILEID_SIZE);
        } 
        
        m_ctx->setState(ENUM_TASK_SUCCESS_END);
    } else {
        m_ctx->fail(-1);
    }
}

