#include<string.h>
#include"uploadcli.h"
#include"msgcenter.h" 
#include"filetool.h"
#include"filereader.h"
#include"uploadeng.h"


static const Char DEF_UPLOAD_DIR[] = "upload";


SenderBase::SenderBase() {
    m_ctx = NULL;
    m_eng = NULL;
    m_reader = NULL;
}

SenderBase::~SenderBase() {
}

Void SenderBase::prepare(I_Ctx* ctx) { 
    Sender* sender = NULL;
    
    sender = dynamic_cast<Sender*>(ctx);
    m_ctx = sender;
    m_reader = sender->reader();
    m_eng = sender->engine();

    prepareEx();
}

Void SenderBase::process(Void* msg) {
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
        LOG_INFO("sender_process| cmd=%d| ret=%d| msg=fail|", cmd, ret);

        /* if remote fail, then go to fail directly without a msg sended */
        data->m_last_error = ret; 
        m_ctx->setState(ENUM_TASK_FAIL_END); 
        
        MsgCenter::freeMsg(msg);
    }
}


Void SenderInit::processEx(Void* msg) {
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

    m_eng->setUpload(TRUE);
    
    data->m_file_size = info->m_file_size;
    data->m_file_crc = info->m_file_crc;
    
    strncpy(data->m_file_name, info->m_file_name, MAX_FILENAME_SIZE);
    strncpy(data->m_file_path, DEF_UPLOAD_DIR, MAX_FILEPATH_SIZE);
}

Void SenderInit::parseDownload(const EvMsgReqDownload* info) {
    TransBaseType* data = m_eng->getData();

    m_eng->setUpload(FALSE);

    data->m_random = info->m_random;
    strncpy(data->m_file_id, info->m_file_id, MAX_FILEID_SIZE);
    strncpy(data->m_file_path, DEF_UPLOAD_DIR, MAX_FILEPATH_SIZE);
}


Void UploadCliConn::prepareEx() {
    Int32 ret = 0;
    EvMsgReqUpload* pReq = NULL;
    TransBaseType* data = m_eng->getData();
    
    LOG_INFO("===========TaskConnUpload::prepare==");

    do { 
        data->m_random = sys_random(); 
        
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

        m_eng->startWatchdog(DEF_CMD_TIMEOUT_SEC); 
        return;
    } while (0);

    m_ctx->fail(ret);
    return;
}

Void UploadCliConn::processEx(Void* msg) { 
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
    } else if (CMD_ALARM_WATCHDOG_TIMEOUT == cmd) {
        LOG_INFO("UploadCliConn::process| msg=timeout|");

        m_ctx->fail(ENUM_ERR_TIMEOUT);
    }

    MsgCenter::freeMsg(msg);
}

Void UploadCliConn::post() {
    m_eng->stopWatchdog(); 
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

        pMsg = buildExchParam();

        ret = m_eng->sendPkg(pMsg);
        if (0 != ret) {
            ret = -3;
            break;
        }

        m_eng->resetWatchdog(DEF_CMD_TIMEOUT_SEC); 

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
        m_eng->setSpeed(localSpeed);
    } else {
        m_eng->setSpeed(info->m_max_speed);
    }
} 


Void DownloadSrvAccept::prepareEx() {
    Int32 ret = 0;
    EvMsgAckDownload* pRsp = NULL;
    TransBaseType* data = m_eng->getData();

    /*  check mac */


    do { 
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

        m_eng->startWatchdog(DEF_CMD_TIMEOUT_SEC); 

        return;
    } while (0);

    m_ctx->fail(ret);
    return; 
}

Void DownloadSrvAccept::processEx(Void* msg) {
    Int32 cmd = 0;
    EvMsgAckParam* pAckParam = NULL;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_ACK_PARAM == cmd) {
        pAckParam = MsgCenter::cast<EvMsgAckParam>(msg);
        
        dealParamAck(pAckParam); 
    } else if (CMD_ALARM_WATCHDOG_TIMEOUT == cmd) {
        LOG_INFO("DownloadSrvAccept::process| msg=timeout|");

        m_ctx->fail(ENUM_ERR_TIMEOUT);
    }

    MsgCenter::freeMsg(msg);
}

Void DownloadSrvAccept::post() {
    m_eng->stopWatchdog(); 
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
        m_eng->setSpeed(localSpeed);
    } else {
        m_eng->setSpeed(info->m_max_speed);
    }
}


Void TaskSetupSender::prepareEx() { 
    m_ctx->prepareSend();

    m_ctx->setState(ENUM_TASK_SLOW_STARTUP);

    //m_ctx->setState(ENUM_TASK_SEND_TEST);
    return;
}

Void TaskSetupSender::processEx(Void* msg) {
    Int32 cmd = 0;

    cmd = MsgCenter::cmd(msg); 

    MsgCenter::freeMsg(msg);
} 

Void TaskSendFinish::prepareEx() {
    Int32 ret = 0;

    do {     
        ret = notifyFinish();
        if (0 != ret) {
            ret = -1;
            break;
        }
        
        m_eng->resetWatchdog(DEF_CMD_TIMEOUT_SEC); 
        return;
    } while (0);

    m_ctx->fail(ret);
    return;
}

Void TaskSendFinish::processEx(Void* msg) {
    Int32 cmd = 0;
    EvMsgTaskCompleted* pRsp = NULL;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_TASK_COMPLETED == cmd) {
        pRsp = MsgCenter::cast<EvMsgTaskCompleted>(msg);
  
        dealFinish(pRsp);
    } else if (CMD_ALARM_WATCHDOG_TIMEOUT == cmd) {
        LOG_INFO("TaskSendFinish::process| msg=timeout|");

        m_ctx->fail(ENUM_ERR_TIMEOUT);
    } else if (CMD_EVENT_REPORT_PARAM == cmd
        || CMD_EVENT_REPORT_PARAM_ACK == cmd) {
        m_eng->parseReportParam(msg); 
    } 

    MsgCenter::freeMsg(msg);
} 

Void TaskSendFinish::post() {
    m_eng->stopWatchdog(); 
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

Void TaskSendFinish::dealFinish(EvMsgTaskCompleted* pRsp) {
    Int32 ret = 0;
    EvMsgTaskCompleted* pMsg = NULL; 
    TransBaseType* data = m_eng->getData(); 

    m_eng->resetWatchdog(DEF_CMD_TIMEOUT_SEC); 
        
    if (m_eng->isUpload()) { 
        strncpy(data->m_file_id, pRsp->m_file_id, MAX_FILEID_SIZE); 
    } else {
        if (0 != strncmp(data->m_file_id, pRsp->m_file_id, MAX_FILEID_SIZE)) {
            m_ctx->fail(-1);
            return;
        }
    }

    pMsg = MsgCenter::creatMsg<EvMsgTaskCompleted>(CMD_TASK_COMPLETED);
    strncpy(pMsg->m_file_id, data->m_file_id, MAX_FILEID_SIZE); 
    
    ret = m_eng->sendPkg(pMsg);
    if (0 == ret) {
        m_ctx->setState(ENUM_TASK_SUCCESS_END);
    } else {
        m_ctx->fail(-1);
    } 
}

Void TaskSlowStartup::prepareEx() {
    TransBaseType* data = m_eng->getData(); 
    
    LOG_INFO("===========TaskSlowStartup::prepare==");

    /* set both frame and window to 1, named as slowstartup */
    data->m_frame_size = 1;
    data->m_wnd_size = 1;
    
    m_fast_counter = 0;
    m_slow_counter = 0;
} 

Void TaskSlowStartup::processEx(Void* msg) {
    Int32 cmd = 0;
    EvMsgTransDataAck* pRsp = NULL;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_TRANS_DATA_BLK_ACK == cmd) {
        pRsp = MsgCenter::cast<EvMsgTransDataAck>(msg); 
        
        dealBlkAck(pRsp);
    } else if (CMD_EVENT_REPORT_PARAM == cmd
        || CMD_EVENT_REPORT_PARAM_ACK == cmd) {
        m_eng->parseReportParam(msg);
    } else if (CMD_EVENT_REPORT_STATUS == cmd) {
        m_ctx->parseReportStatus(msg);
    } else if (CMD_ALARM_TRANS_BLK == cmd) {
        m_ctx->fail(ENUM_ERR_READ_BLK);
    } else if (CMD_ALARM_WATCHDOG_TIMEOUT == cmd) {
        LOG_INFO("TaskSlowStartup::process| msg=retrans timeout|");

        m_ctx->setState(ENUM_TASK_RETRANSMISSION);
    }

    MsgCenter::freeMsg(msg);
}

Void TaskSlowStartup::dealBlkAck(EvMsgTransDataAck* pRsp) {
    Bool bChg = FALSE;
    TransBaseType* data = m_eng->getData(); 

    if (data->m_blk_beg <= pRsp->m_blk_next
        && data->m_blk_end >= pRsp->m_blk_next) { 
        
        if (data->m_blk_next_ack < pRsp->m_blk_next) { 
            /* reset the timer */
            m_eng->resetWatchdog(DEF_RETRANS_INTERVAL_SEC);
            
            if (0 != data->m_dup_ack_cnt) {
                data->m_dup_ack_cnt = 0;
            }

            m_reader->purgeWaitList(pRsp, data); 

            bChg = correct(pRsp, data);
            if (bChg) { 
                m_ctx->setState(ENUM_TASK_AVOID_CONG_SENDER);
            }
        } else if (data->m_blk_next_ack == pRsp->m_blk_next
            && data->m_blk_next_ack < data->m_blk_cnt) {
            /* duplicated ack for 3 times means  a fast recover */
            if (DEF_FAST_RECOVER_CNT <= ++data->m_dup_ack_cnt) {

                data->m_dup_ack_cnt = 0; 
                m_ctx->setState(ENUM_TASK_RETRANSMISSION);
            }
        }
    }
}

Bool TaskSlowStartup::correct(const EvMsgTransDataAck* pRsp, 
    TransBaseType* data) {
    Int32 max_frame = 0; 

    max_frame = m_eng->calcMaxFrame();
    if (data->m_frame_size >= max_frame) {
        data->m_frame_size = max_frame;
        return TRUE;
    } 
    
    if (pRsp->m_time + DEF_INC_SPEED_TICK_CNT >= m_eng->now()) { 
        if (0 != m_slow_counter) {
            m_slow_counter = 0;
        }
        
        ++m_fast_counter;
        
        /* if there has 2 continuos fast ack, then multiple frame size */
        if (m_fast_counter >= DEF_INC_CONTINUOS_CNT) {
            m_fast_counter= 0;
            
            data->m_frame_size <<= 1;

            if (data->m_frame_size >= max_frame) {
                data->m_frame_size = max_frame;

                return TRUE;
            } 
        }
    } else if (pRsp->m_time + DEF_DEC_SPEED_TICK_CNT <= m_eng->now()) {
        if (0 != m_fast_counter) {
            m_fast_counter = 0;
        }
        
        ++m_slow_counter;

        if (m_slow_counter >= DEF_DEC_CONTINUOS_CNT) {
            /* if there has 3 continuos slow ack, then retain frame size */
            return TRUE;
        }
    } else {
        if (0 != m_fast_counter) {
            m_fast_counter = 0;
        }

        if (0 != m_slow_counter) {
            m_slow_counter = 0;
        }
    }

    return FALSE;
}

Void TaskSlowStartup::dealQuarterTimer() { 
    sendBlk(); 
}

Void TaskSlowStartup::dealReportTimer() {
    m_eng->sendReportParam();
}

Void TaskSlowStartup::sendBlk() {
    Int32 ret = 0;
    Bool bOk = TRUE;
    TransBaseType* data = m_eng->getData();

    if (m_reader->waitSize() < data->m_wnd_size) {
        bOk = m_reader->pushBlk(data); 
    }

    ret = m_reader->sendBlk(data);
    if (0 == ret) {
        /* check if all acked as end of send */
        if (data->m_blk_next_ack == data->m_blk_end) {
            m_ctx->setState(ENUM_TASK_SEND_FINISH);
        }
    } else {
        m_ctx->fail(ret);
    } 
}


Void TaskAvoidCongest::prepareEx() {
    
    LOG_INFO("===========TaskAvoidCongest::prepare==");

    m_send_cnt = 0;
}

Void TaskAvoidCongest::processEx(Void* msg) {
    Int32 cmd = 0;
    EvMsgTransDataAck* pRsp = NULL;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_TRANS_DATA_BLK_ACK == cmd) {
        pRsp = MsgCenter::cast<EvMsgTransDataAck>(msg);
        
        dealBlkAck(pRsp);
    } else if (CMD_EVENT_REPORT_PARAM == cmd
        || CMD_EVENT_REPORT_PARAM_ACK == cmd) {
        parseReportParam(msg);
    } else if (CMD_EVENT_REPORT_STATUS == cmd) {
        m_ctx->parseReportStatus(msg);
    } else if (CMD_ALARM_TRANS_BLK == cmd) {
        m_ctx->fail(ENUM_ERR_READ_BLK);
    } else if (CMD_ALARM_WATCHDOG_TIMEOUT == cmd) {
        LOG_INFO("TaskAvoidCongest::process| msg=retrans timeout|");

        m_ctx->setState(ENUM_TASK_RETRANSMISSION);
    }

    MsgCenter::freeMsg(msg);
}

Void TaskAvoidCongest::parseReportParam(Void* msg) {
    TransBaseType* data = m_eng->getData(); 

    m_eng->parseReportParam(msg);

    if (chkAutoconf(data)) {
        m_ctx->setState(ENUM_TASK_SLOW_STARTUP);
    }
}

Bool TaskAvoidCongest::chkAutoconf(TransBaseType* data) {

    if (data->m_frame_size < m_eng->calcMaxFrame()) {
        if (data->m_realtime_ratio >= (data->m_frame_size << 1)
            || data->m_realtime_ratio >= m_eng->calcMaxFrame()) {
            return TRUE;
        }
    } else if (data->m_frame_size > m_eng->calcMaxFrame()) {
        return TRUE;
    }

    return FALSE;
}

Void TaskAvoidCongest::dealQuarterTimer() { 
    if (0 != m_send_cnt) {
        m_send_cnt = 0;
    }

    sendBlk(); 
}

Void TaskAvoidCongest::dealReportTimer() {
    m_eng->sendReportParam();
}

Void TaskAvoidCongest::dealBlkAck(EvMsgTransDataAck* pRsp) {
    TransBaseType* data = m_eng->getData(); 

    if (data->m_blk_beg <= pRsp->m_blk_next
        && data->m_blk_end >= pRsp->m_blk_next) {
        
        if (data->m_blk_next_ack < pRsp->m_blk_next) {
            /* reset the timer */
            m_eng->resetWatchdog(DEF_RETRANS_INTERVAL_SEC);
            
            if (0 != data->m_dup_ack_cnt) {
                data->m_dup_ack_cnt = 0;
            }
            
            m_reader->purgeWaitList(pRsp, data); 

            if (data->m_wnd_size < m_eng->calcMaxWnd()) {
                ++data->m_wnd_size;
            }
            
            sendBlk();
        } else if (data->m_blk_next_ack == pRsp->m_blk_next
            && data->m_blk_next_ack < data->m_blk_cnt) {
            /* duplicated ack for 3 times means  a fast recover */
            if (DEF_FAST_RECOVER_CNT <= ++data->m_dup_ack_cnt) {

                data->m_dup_ack_cnt = 0;
                m_ctx->setState(ENUM_TASK_RETRANSMISSION);
            }
        } else {
            /* unused ack, ignore it */
        }
    }
}

Void TaskAvoidCongest::sendBlk() {
    Int32 ret = 0;
    Bool bOk = TRUE;
    TransBaseType* data = m_eng->getData();

    while (m_send_cnt + data->m_frame_size <= m_eng->quarterSpeed()
        && m_reader->waitSize() < data->m_wnd_size) {
        bOk = m_reader->pushBlk(data); 
        if (bOk) { 
            m_send_cnt += data->m_frame_size;
        } else {
            break;
        }
    }

    ret = m_reader->sendBlk(data);
    if (0 == ret) {
        /* check if all acked as end of send */
        if (data->m_blk_next_ack == data->m_blk_end) {
            m_ctx->setState(ENUM_TASK_SEND_FINISH);
        }
    } else {
        m_ctx->fail(ret);
    } 
}


Void TaskRetrans::prepareEx() { 
    
    LOG_INFO("===========TaskRetrans::prepare==");

    m_retrans_cnt = 0;
    m_allow = TRUE;

    if (NULL != m_reader->getLastWait()) {
        m_eng->resetWatchdog(DEF_RETRANS_INTERVAL_SEC); 
    } else {
        m_ctx->setState(ENUM_TASK_SLOW_STARTUP); 
    }
}

Void TaskRetrans::processEx(Void* msg) {
    Int32 cmd = 0;
    EvMsgTransDataAck* pRsp = NULL;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_TRANS_DATA_BLK_ACK == cmd) {
        pRsp = MsgCenter::cast<EvMsgTransDataAck>(msg);
        
        dealBlkAck(pRsp);
    } else if (CMD_EVENT_REPORT_PARAM == cmd
        || CMD_EVENT_REPORT_PARAM_ACK == cmd) {
        m_eng->parseReportParam(msg);
    } else if (CMD_EVENT_REPORT_STATUS == cmd) {
        m_ctx->parseReportStatus(msg);
    } else if (CMD_ALARM_TRANS_BLK == cmd) {
        m_ctx->fail(ENUM_ERR_READ_BLK);
    } else if (CMD_ALARM_WATCHDOG_TIMEOUT == cmd) {
        dealRetrans();
    }

    MsgCenter::freeMsg(msg);
}

Void TaskRetrans::dealQuarterTimer() { 
    if (m_allow) {
        m_allow = FALSE;

        if (DEF_MAX_RETRANS_CNT > ++m_retrans_cnt) {
            retransmission(); 
        } else {
            m_ctx->fail(-1);
        }
    } 
}

Void TaskRetrans::dealReportTimer() {
    m_eng->sendReportParam();
}

Void TaskRetrans::dealRetrans() {
    LOG_INFO("TaskRetrans::dealRetrans| msg=retrans timeout|");

    m_eng->startWatchdog(DEF_RETRANS_INTERVAL_SEC); 
    
    m_allow = TRUE;
}

Void TaskRetrans::dealBlkAck(EvMsgTransDataAck* pRsp) {
    EvMsgBlkRange* pBlk = NULL;
    TransBaseType* data = m_eng->getData(); 

    if (data->m_blk_beg <= pRsp->m_blk_next
        && data->m_blk_end >= pRsp->m_blk_next) {
          
        if (data->m_blk_next_ack < pRsp->m_blk_next) {
            m_eng->resetWatchdog(DEF_RETRANS_INTERVAL_SEC); 

            if (0 != data->m_dup_ack_cnt) {
                data->m_dup_ack_cnt = 0;
            }

            m_retrans_cnt = 0;
            m_allow = TRUE;

            pBlk = m_reader->getLastWait();      
            if (pBlk->m_blk_end <= pRsp->m_blk_next) {
                m_reader->purgeWaitList(pRsp, data);

                if (!needRetrans()) {
                    m_ctx->setState(ENUM_TASK_SLOW_STARTUP); 
                }
            } 
        } 
    }
}

Void TaskRetrans::retransmission() {
    Int32 ret = 0;
    EvMsgBlkRange* pBlk = NULL;
    TransBaseType* data = m_eng->getData(); 
    
    pBlk = m_reader->getLastWait();
    if (NULL != pBlk) {
        /* here cannot check time for fast recovery isnot timeout yet */
        ret = m_reader->retransFrame(pBlk, data);
        if (0 != ret) {
            m_ctx->fail(-1);
        }
    } else {
        m_ctx->setState(ENUM_TASK_SLOW_STARTUP); 
    }
}

Bool TaskRetrans::needRetrans() {
    EvMsgBlkRange* pBlk = NULL;

    pBlk = m_reader->getLastWait();
    if (NULL == pBlk 
        || (pBlk->m_time + DEF_RETRANS_INTERVAL_SEC > m_eng->now())) {
        return FALSE;
    } else {
        return TRUE;
    }
} 


Void TaskSendTest::prepareEx() {
    TransBaseType* data = m_eng->getData(); 
    
    LOG_INFO("===========TaskSendTest::prepare==");

    /* set both frame and window to 1, named as slowstartup */
    data->m_frame_size = m_eng->calcMaxFrame();
    data->m_wnd_size = 1;
} 

Void TaskSendTest::processEx(Void* msg) {
    Int32 cmd = 0;
    EvMsgTransDataAck* pRsp = NULL;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_TRANS_DATA_BLK_ACK == cmd) {
        pRsp = MsgCenter::cast<EvMsgTransDataAck>(msg); 
        
        dealBlkAck(pRsp);
    } else if (CMD_EVENT_REPORT_PARAM == cmd
        || CMD_EVENT_REPORT_PARAM_ACK == cmd) {
        m_eng->parseReportParam(msg);
    } else if (CMD_EVENT_REPORT_STATUS == cmd) {
        m_ctx->parseReportStatus(msg);
    } else if (CMD_ALARM_TRANS_BLK == cmd) {
        m_ctx->fail(ENUM_ERR_READ_BLK);
    } else if (CMD_ALARM_WATCHDOG_TIMEOUT == cmd) {
        LOG_INFO("TaskSendTest::process| msg=retrans timeout|");

        m_ctx->fail(ENUM_ERR_TIMEOUT);
    }

    MsgCenter::freeMsg(msg);
}

Void TaskSendTest::dealBlkAck(EvMsgTransDataAck* pRsp) {
    TransBaseType* data = m_eng->getData(); 

    if (data->m_blk_beg <= pRsp->m_blk_next
        && data->m_blk_end >= pRsp->m_blk_next) { 
        
        if (data->m_blk_next_ack < pRsp->m_blk_next) { 
            /* reset the timer */
            m_eng->resetWatchdog(DEF_RETRANS_INTERVAL_SEC);
            
            m_reader->purgeWaitList(pRsp, data); 
        } 
    }
}

Void TaskSendTest::dealQuarterTimer() { 
    sendBlk(); 
}

Void TaskSendTest::dealReportTimer() {
    m_eng->sendReportParam();
}

Void TaskSendTest::sendBlk() {
    Int32 ret = 0;
    Bool bOk = TRUE;
    TransBaseType* data = m_eng->getData();

    if (m_reader->waitSize() < data->m_wnd_size) {
        bOk = m_reader->pushBlk(data); 
    }

    ret = m_reader->sendBlk(data);
    if (0 == ret) {
        /* check if all acked as end of send */
        if (data->m_blk_next_ack == data->m_blk_end) {
            m_ctx->setState(ENUM_TASK_SEND_FINISH);
        }
    } else {
        m_ctx->fail(ret);
    } 
}


