#include<string.h>
#include"engine.h"
#include"msgcenter.h"
#include"cmdctx.h"
#include"ictx.h"


Engine::Engine(const Char* id) {
    m_env = NULL;
        
    m_data = NULL;
    m_ctx = NULL; 

    m_waitdog_timer = NULL;
    m_quarter_timer = NULL;
    m_report_timer = NULL;
    
    m_update_speed = FALSE;
    m_upload = FALSE;
    m_send = FALSE;

    memset(m_task_id, 0, MAX_TASKID_SIZE);
    strncpy(m_task_id, id, MAX_TASKID_SIZE);

    INIT_LIST_NODE(&m_item.m_id_node);
    INIT_LIST_NODE(&m_item.m_run_node);
    m_item.m_eng = this;
    m_item.m_is_run = FALSE;
}

Engine::~Engine() { 
}

Int32 Engine::start() {
    Int32 ret = 0;

    if (NULL == m_ctx || NULL == m_env || '\0' == m_task_id[0]) {
        return -1;
    } 

    I_NEW(TransBaseType, m_data);
    resetTransData(m_data);

    m_conf_speed = 0;
    m_update_speed = FALSE;

    m_waitdog_timer = m_env->creatTimer(this, CMD_ALARM_WATCHDOG_TIMEOUT);
    if (NULL == m_waitdog_timer) {
        return -1;
    } 

    m_quarter_timer = m_env->creatTimer(this, CMD_ALARM_TICK_QUARTER_SEC);
    if (NULL == m_quarter_timer) {
        return -1;
    }

    m_report_timer = m_env->creatTimer(this, CMD_ALARM_REPORT_TIMEOUT);
    if (NULL == m_quarter_timer) {
        return -1;
    }
    
    ret = m_ctx->start();
    if (0 != ret) {
        return ret;
    }

    ret = QueWorker::start();
    
    return ret;
}

Void Engine::stop() {
    QueWorker::stop();
    
    if (NULL != m_ctx) {
        m_ctx->stop();

        I_FREE(m_ctx);
    }

    if (NULL != m_quarter_timer) {
        m_env->delTimer(m_quarter_timer);

        m_quarter_timer = NULL;
    }

    if (NULL != m_waitdog_timer) {
        m_env->delTimer(m_waitdog_timer);

        m_waitdog_timer = NULL;
    }

    if (NULL != m_report_timer) {
        m_env->delTimer(m_report_timer);

        m_report_timer = NULL;
    }
    
    I_FREE(m_data);
}

Void Engine::dealEvent(Void* msg) {
    m_ctx->procEvent(msg);
}

const Char* Engine::getID() const {
    return m_task_id;
}

Void Engine::setUpload(Bool is_upload) {
    m_upload = is_upload;
}

Bool Engine::isUpload() const {
    return m_upload;
}

Void Engine::setSend(Bool is_send) {
    m_send = is_send;
}

Bool Engine::isSend() const {
    return m_send;
}

Void Engine::setCtx(I_Ctx* ctx) {
    m_ctx = ctx;
}

Void Engine::setEnv(CmdCtx* env) {
    m_env = env;
}

Uint32 Engine::now() const {
    return m_env->now();
}

Uint32 Engine::monoTick() const {
    return m_env->monoTick();
}

TransBaseType* Engine::getData() const {
    return m_data;
}

Int32 Engine::getConfWnd() const {
    TaskConfType* conf = m_env->getConf();

    return conf->m_max_cwnd;
}

Int32 Engine::getConfMss() const {
    TaskConfType* conf = m_env->getConf();

    return conf->m_mss;
}

Int32 Engine::calcConfSpeed() const {
    if (m_send) {
        return m_env->getSendSpeed();
    } else {
        return m_env->getRecvSpeed();
    } 
}

Int32 Engine::getConfSpeed() const {
    return m_conf_speed;
}

Void Engine::updateSpeed() {
    Int32 calc = calcConfSpeed();
    
    if (m_conf_speed != calc) {
        m_conf_speed = calc;

        if (!m_update_speed) {
            m_update_speed = TRUE;
        }
    }
}

Void Engine::setSpeed(Int32 s) {
    if (m_data->m_max_speed != s) {
        m_data->m_max_speed = s;

        m_data->m_quarter_speed = (m_data->m_max_speed >> 2);
        if (0 == m_data->m_quarter_speed) {
            m_data->m_quarter_speed = 1;
        }
    } 
}

Int32 Engine::speed() const {
    return m_data->m_max_speed;
}

Int32 Engine::quarterSpeed() const {
    return m_data->m_quarter_speed;
}

Int32 Engine::sendPkg(Void* msg) {
    Int32 ret = 0;

    MsgCenter::setID(getID(), msg);
    ret = m_env->sendPkg(msg);
    return ret;
}

Int32 Engine::transPkg(Void* msg) {
    Int32 ret = 0;

    MsgCenter::setID(getID(), msg);
    ret = m_env->transPkg(msg);
    return ret;
}

Bool Engine::notify(Void* msg) {
    Bool bOk = TRUE;
 
    bOk = QueWorker::notify(msg);
    if (bOk) {
        m_env->addRunList(this);
    }

    return bOk;
}

Bool Engine::emerge(Void* msg) { 
    Bool bOk = TRUE;

    bOk = QueWorker::emerge(msg);
    if (bOk) {
        m_env->addRunList(this);
    }

    return bOk;
}

Void Engine::fillReport(ReportFileInfo* info) {

    info->m_file_size = m_data->m_file_size;
    info->m_result = m_data->m_last_error;
    info->m_upload_download = m_upload;
    info->m_send_recv = m_send;

    strncpy(info->m_file_path, m_data->m_file_path, MAX_FILEPATH_SIZE);
    strncpy(info->m_file_name, m_data->m_file_name, MAX_FILENAME_SIZE);
    strncpy(info->m_file_id, m_data->m_file_id, MAX_FILEID_SIZE);
    strncpy(info->m_task_id, getID(), MAX_TASKID_SIZE);
}

Void Engine::reportEnd() {
    I_Event* info = NULL;
    
    info = MsgCenter::creatMsg<I_Event>(CMD_FILE_TRANS_RESULT_REPORT);
    
    MsgCenter::setID(getID(), info);
    m_env->recvPkg(info);
}

Void Engine::fail2Peer(Int32 error) {
    I_Event* info = NULL;
    
    info = MsgCenter::creatMsg<I_Event>(CMD_ERROR_PEER_FAIL);

    MsgCenter::setError(error, info);
    
    sendPkg(info);
}

Void Engine::startWatchdog(Uint32 tick) {
    m_env->startTimer(m_waitdog_timer, tick);
}

Void Engine::stopWatchdog() {
    m_env->stopTimer(m_waitdog_timer);
}

Void Engine::resetWatchdog(Uint32 tick) {
    m_env->stopTimer(m_waitdog_timer);
    m_env->startTimer(m_waitdog_timer, tick);
}

Void Engine::startQuarterSec() {    
    m_env->startTimer(m_quarter_timer, DEF_QUARTER_SEC_TICK_CNT);
}

Void Engine::stopQuarterSec() {
    m_env->stopTimer(m_quarter_timer);
}

Void Engine::startReportTimer() {    
    m_env->startTimer(m_report_timer, DEF_REPORT_PROGRESS_INTERVAL);
}

Void Engine::stopReportTimer() {
    m_env->stopTimer(m_report_timer);
}

Int32 Engine::calcMaxFrame() const {
    if (m_data->m_quarter_speed <= DEF_MAX_FRAME_SIZE) {
        return m_data->m_quarter_speed;
    } else {
        return DEF_MAX_FRAME_SIZE;
    }
}

Int32 Engine::calcMaxWnd() const {
    Int32 cnt = 0;
    Int32 val = 0;

    val = m_data->m_max_speed;

    if (DEF_SPEED_SLOW_SIZE > val) {
        return DEF_SLOW_CWND_SIZE;
    } else if (DEF_SPEED_MEDIUM_SIZE > val) {
        /* increment 1 by every 2MB */
        return DEF_STEP_1_CWND_SIZE + (val >> 9);
    } else {
        /* increment 1 every 4MB */
        cnt = DEF_STEP_2_CWND_SIZE + (val >> 10);
        if (cnt <= DEF_MAX_CWND_SIZE) {
            return cnt;
        } else {
            return DEF_MAX_CWND_SIZE;
        }
    }
}

Void Engine::parseReportParam(Void* msg) {
    Int32 cmd = 0;
    Int32 oldSpeed = 0;
    Int32 confSpeed = 0;
    EvMsgReportParam* pReq = NULL;

    cmd = MsgCenter::cmd(msg); 
    pReq = MsgCenter::cast<EvMsgReportParam>(msg);

    oldSpeed = speed();
    confSpeed = getConfSpeed();
    if (confSpeed <= pReq->m_max_speed) {
        if (oldSpeed != confSpeed) {
            setSpeed(confSpeed);
        }
    } else {
        if (oldSpeed != pReq->m_max_speed) {
            setSpeed(pReq->m_max_speed);
        }
    }

    if (CMD_EVENT_REPORT_PARAM == cmd) {
        sendReportParamAck();
    }

    LOG_INFO("parse_report_param| cmd=%d| conf=%d|"
        " remote=%d| old_speed=%d| new_speed=%d|",
        cmd, confSpeed, pReq->m_max_speed, 
        oldSpeed, speed());
}

Void Engine::sendReportParam() {
    EvMsgReportParam* pReq = NULL; 

    if (m_update_speed) {
        pReq = MsgCenter::creatMsg<EvMsgReportParam>(CMD_EVENT_REPORT_PARAM);
        pReq->m_max_speed = getConfSpeed();
        
        sendPkg(pReq); 

        m_update_speed = FALSE;
    }
}

Void Engine::sendReportParamAck() {
    EvMsgReportParam* pReq = NULL; 
    
    pReq = MsgCenter::creatMsg<EvMsgReportParam>(CMD_EVENT_REPORT_PARAM_ACK);
    pReq->m_max_speed = getConfSpeed();
    
    sendPkg(pReq); 
}



