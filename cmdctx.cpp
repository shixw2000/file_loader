#include<string.h>
#include"ievent.h"
#include"cmdctx.h"
#include"msgcenter.h"
#include"filetool.h"
#include"ticktimer.h"
#include"uploadeng.h"
#include"downloadeng.h"
#include"ihandler.h"
#include"engine.h"


static int cmpEngID(list_node* n1, list_node* n2) {
    EngineItem* item1 = list_entry(n1, EngineItem, m_id_node);
    EngineItem* item2 = list_entry(n2, EngineItem, m_id_node);

    return strcmp(item1->m_eng->getID(), item2->m_eng->getID());
}


CmdCtx::CmdCtx() { 
    m_data = NULL;
    m_timer = NULL;
    m_comm = NULL;

    INIT_ORDER_LIST_HEAD(&m_id_container, &cmpEngID);
    INIT_LIST_HEAD(&m_run_container);
}

CmdCtx::~CmdCtx() {
}

void CmdCtx::setParam(TickTimer* timer, I_Comm* comm) { 
    m_timer = timer;
    m_comm = comm;
}

void CmdCtx::setSpeed(Int32 sndSpeed, Int32 rcvSpeed) {
    m_data->m_conf_send_speed = sndSpeed; 
    m_data->m_conf_recv_speed = rcvSpeed;
}

Int32 CmdCtx::start() {    
    Int32 ret = 0;
    
    I_NEW(TaskConfType, m_data); 
    resetTaskConf(m_data);

    ret = QueWorker::start();
    if (0 != ret) {
        return ret;
    }

    return ret;
}

Void CmdCtx::stop() {     
    QueWorker::stop();

    freeAllEngine();
    
    I_FREE(m_data); 
}

Int32 CmdCtx::sendPkg(Void* msg) {
    return m_comm->sendPkg(msg);
}

Int32 CmdCtx::transPkg(Void* msg) {
    return m_comm->transPkg(msg);
}

Int32 CmdCtx::recvPkg(Void* msg) {
    Bool bOk = TRUE;
    
    bOk = notify(msg);
    if (bOk) {
        m_comm->signal();
    }
    
    return 0;
}

Uint32 CmdCtx::now() const {
    return m_timer->now();
}

Uint32 CmdCtx::monoTick() const {
    return m_timer->monoTick();
}

Int32 CmdCtx::getSendSpeed() const {
    if (0 == m_data->m_send_thr_cnt) {
        return m_data->m_conf_send_speed;
    } else {
        return m_data->m_conf_send_speed / m_data->m_send_thr_cnt;
    }
}

Int32 CmdCtx::getRecvSpeed() const {
    if (0 == m_data->m_recv_thr_cnt) {
        return m_data->m_conf_recv_speed;
    } else {
        return m_data->m_conf_recv_speed / m_data->m_recv_thr_cnt;
    }
}

Int32 CmdCtx::uploadFile(EvMsgStartUpload* msg) { 
    Int32 ret = 0;
    
    ret = setup(msg);
    return ret; 
}

Int32 CmdCtx::downloadFile(EvMsgStartDownload* msg) { 
    Int32 ret = 0;
    
    ret = setup(msg);
    return ret; 
} 

Void CmdCtx::dealEvent(Void* msg) {
    Int32 cmd = 0;
    const Char* id = NULL;
    Engine* eng = NULL;

    id = MsgCenter::ID(msg);
    cmd = MsgCenter::cmd(msg);

    if (CMD_FILE_TRANS_RESULT_REPORT != cmd) {
        eng = findEngine(id);
        if (NULL != eng) {
            if (CMD_REQ_FILE_UPLOAD != cmd
                && CMD_REQ_FILE_DOWNLOAD != cmd) {
                eng->notify(msg);
            } else {
                MsgCenter::freeMsg(msg);
            }
        } else if (CMD_REQ_FILE_UPLOAD == cmd) {
            setup(msg);
        } else if (CMD_REQ_FILE_DOWNLOAD == cmd) {
            setup(msg);
        } else {
            MsgCenter::freeMsg(msg);
        }
        
    } else {
        doReportResult(id);
        MsgCenter::freeMsg(msg);
    } 
} 

Void CmdCtx::activeTimer(Void* p1, Void* p2) {
    I_Event* ev = NULL;
    QueWorker* que = reinterpret_cast<QueWorker*>(p1);
    Int32 type = (Int32)(Int64)(p2);
    
    ev = MsgCenter::creatMsg<I_Event>(type); 
    que->notify(ev);
}

Void* CmdCtx::creatTimer(QueWorker* que, Int32 type) {
    return m_timer->creatTimer(&CmdCtx::activeTimer, que, (Void*)type);
}

Void CmdCtx::delTimer(Void* id) {
    m_timer->delTimer(id);
}

Void CmdCtx::resetTimer(Void* id, Uint32 tick) {
    m_timer->resetTimer(id, tick);
}

Void CmdCtx::startTimer(Void* id, Uint32 tick) {
    m_timer->startTimer(id, tick);
}

Void CmdCtx::stopTimer(Void* id) {
    m_timer->stopTimer(id);
} 

Void CmdCtx::doReportResult(const Char* id) {
    Engine* eng = NULL;
    ReportFileInfo info;

    eng = findEngine(id);
    if (NULL != eng) { 
        /* clear a ended task */
        eng->fillReport(&info); 

        delEngine(eng);

        m_comm->reportResult(&info);
    }
}

Engine* CmdCtx::findEngine(const Char* id) {
    list_node* node = NULL;
    EngineItem* item = NULL;
    Engine oEng(id);

    node = order_list_find(&oEng.getItem()->m_id_node, &m_id_container);
    if (NULL != node) {
        item = list_entry(node, EngineItem, m_id_node);
        return item->m_eng;
    } else {
        return NULL;
    }
}

Bool CmdCtx::exists(const Char* id) {
    list_node* node = NULL;
    Engine oEng(id);

    node = order_list_find(&oEng.getItem()->m_id_node, &m_id_container);
    if (NULL != node) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool CmdCtx::delEngine(Engine* eng) {
    Bool isSend = FALSE;
    
    order_list_del(&eng->getItem()->m_id_node, &m_id_container);
    delRunList(eng); 
    
    if (eng->isSend()) {
        isSend = TRUE; 
        --m_data->m_send_thr_cnt; 
    } else {
        isSend = FALSE;
        --m_data->m_recv_thr_cnt;
    }

    eng->stop();
    I_FREE(eng);

    updateSpeed(isSend);
    return TRUE;
}

Int32 CmdCtx::setup(Void* msg) { 
    Int32 ret = 0;
    Int32 cmd = 0;
    Bool bFound = FALSE;
    Engine* eng = NULL;
    const Char* id = NULL;

    id = MsgCenter::ID(msg);
    cmd = MsgCenter::cmd(msg);

    bFound = exists(id);
    if (!bFound) {
        eng = creatEngine(cmd, id);
        if (NULL != eng) {
            ret = addEngine(eng);
            if (0 == ret) {
                eng->notify(msg);
                return 0;
            } else {
                eng->stop();
                I_FREE(eng);

                MsgCenter::freeMsg(msg); 
                return -1;
            }
        } else {
            MsgCenter::freeMsg(msg);
            return -1;
        } 
    } else {
        MsgCenter::freeMsg(msg);
        return -1;
    } 
}

Engine* CmdCtx::creatEngine(Int32 cmd, const Char* id) {
    Engine* eng = NULL;

    switch (cmd) {
    case CMD_REQ_FILE_UPLOAD:
    case CMD_START_FILE_DOWNLOAD:
        I_NEW_2(ReceiverEngine, eng, this, id);
        break;

    case CMD_REQ_FILE_DOWNLOAD:
    case CMD_START_FILE_UPLOAD:
        I_NEW_2(SenderEngine, eng, this, id);
        break;

    default:
        break;
    }

    return eng;
}

Int32 CmdCtx::addEngine(Engine* eng) {
    Bool isSend = FALSE;
    Int32 ret = 0;
    Int32* pCnt = NULL;
 
    if (eng->isSend()) {
        isSend = TRUE;
        
        if (m_data->m_send_thr_cnt < m_data->m_send_thr_max) {
            pCnt = &m_data->m_send_thr_cnt;
        } else {
            return -1;
        }
        
    } else {
        isSend = FALSE;
        
        if (m_data->m_recv_thr_cnt < m_data->m_recv_thr_max) {
            pCnt = &m_data->m_recv_thr_cnt;
        } else {
            return -1;
        }
    }

    ret = eng->start();
    if (0 != ret) {
        return ret;
    }

    order_list_add(&eng->getItem()->m_id_node, &m_id_container);

    ++(*pCnt); 

    updateSpeed(isSend);
    return 0;
}

Void CmdCtx::updateSpeed(Bool isSend) {
    EngineItem* pos = NULL;
    Engine* eng = NULL;

    list_for_each_entry(pos, &m_id_container, m_id_node) {
        eng = pos->m_eng;
        if (!(eng->isSend() ^ isSend)) {
            eng->updateSpeed();
        }
    }
}

Void CmdCtx::addRunList(Engine* eng) {
    EngineItem* item = eng->getItem();
    
    if (!item->m_is_run) {
        list_add_back(&item->m_run_node, &m_run_container);
        item->m_is_run = TRUE;

        m_comm->signal();
    } 
}

Void CmdCtx::delRunList(Engine* eng) {
    EngineItem* item = eng->getItem();

    if (item->m_is_run) {
        list_del(&item->m_run_node, &m_run_container);
        item->m_is_run = FALSE;
    }
}

Void CmdCtx::doTasks() {
    EngineItem* pos = NULL;
    Engine* eng = NULL;

    while (!list_empty(&m_run_container)) {
        pos = list_first_entry(&m_run_container, EngineItem, m_run_node);
        list_del(&pos->m_run_node, &m_run_container); 
        
        eng = pos->m_eng;
        eng->consume();

        pos->m_is_run = FALSE;
    }
}

Void CmdCtx::run() {    
    consume();
    doTasks();
}

Void CmdCtx::freeAllEngine() {
    EngineItem* pos = NULL;
    Engine* eng = NULL;

    while (!order_list_empty(&m_id_container)) {
        /* delete from id container */
        pos = list_first_entry(&m_id_container, EngineItem, m_id_node);
        order_list_del(&pos->m_id_node, &m_id_container); 

        if (pos->m_is_run) {
            /* delete from run container */
            list_del(&pos->m_run_node, &m_run_container);
            pos->m_is_run = FALSE;
        }
        
        eng = pos->m_eng;
        eng->stop();
        I_FREE(eng);
    }
}

Void CmdCtx::test_01() {
    Void* msg = NULL;

    msg = test_upload();
    if (NULL != msg) {
        uploadFile((EvMsgStartUpload*)msg);
    }
}

Void CmdCtx::test_02() {
    Void* msg = NULL;

    msg = test_download();
    if (NULL != msg) {
        downloadFile((EvMsgStartDownload*)msg);
    }
}


Void* test_upload() {
    Int32 ret = 0;
    char path[] = "upload/test_file";
    char taskId[] = "task_id_123456";
    EvMsgStartUpload* pMsg = NULL;
    Char* pname = NULL;
    FileInfoType info;

    ret = FileTool::statFile(path, &info);
    if (0 == ret) {
        pname = basename(path);
        
        pMsg = MsgCenter::creatMsg<EvMsgStartUpload>(CMD_START_FILE_UPLOAD);
        
        pMsg->m_file_size = info.m_file_size;;
        pMsg->m_file_crc = 0;
        strncpy(pMsg->m_file_name, pname, MAX_FILENAME_SIZE);

        MsgCenter::setID(taskId, pMsg);
        
        return pMsg;
    } else {
        return NULL;
    }
}

Void* test_download() {
    char fileId[] = "task_id_123456";
    char taskId[] = "test_02_task";
    EvMsgStartDownload* pMsg = NULL;

    pMsg = MsgCenter::creatMsg<EvMsgStartDownload>(CMD_START_FILE_DOWNLOAD);
    
    strncpy(pMsg->m_file_id, fileId, sizeof(pMsg->m_file_id));
    
    MsgCenter::setID(taskId, pMsg);
    
    return pMsg;
}

