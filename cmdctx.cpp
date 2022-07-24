#include<string.h>
#include"istate.h"
#include"ictx.h"
#include"ievent.h"
#include"cmdctx.h"
#include"eventmsg.h"
#include"msgcenter.h"
#include"filetool.h"
#include"ticktimer.h"
#include"taskstat.h"
#include"uploadeng.h"
#include"downloadeng.h"


bool CmdCtx::KeyCmp::operator()(const char* s1, const char* s2) {
    return 0 > strcmp(s1, s2);
}

CmdCtx::CmdCtx() {
    m_data = NULL;
    m_timer = NULL;
}

CmdCtx::~CmdCtx() {
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
    
    I_FREE(m_data); 
}

Uint32 CmdCtx::now() const {
    return m_timer->now();
}

Int32 CmdCtx::uploadFile(EvMsgStartUpload* msg) { 
    Int32 ret = 0;
    Engine* eng = NULL;
    const Char* id = NULL;
    typeTabItr itr;

    id = MsgCenter::ID(msg); 
    
    itr = m_container.find(id);
    if (m_container.end() == itr) {
        I_NEW_1(SenderEngine, eng, this);
        ret = eng->start();
        if (0 != ret) {
            return ret;
        } 

        eng->setID(id);
        eng->notify(msg);
        m_container[eng->getID()] = eng; 

        return 0;
    } else {
        /* the task is already running, cannot setup again */
        return -1;
    }
}

Int32 CmdCtx::acceptUpload(Void* msg) { 
    Int32 ret = 0;
    Engine* eng = NULL;
    const Char* id = NULL;

    id = MsgCenter::ID(msg); 

    I_NEW_1(ReceiverEngine, eng, this);
    ret = eng->start();
    if (0 != ret) {
        return ret;
    }
    
    eng->setID(id);
    eng->notify(msg); 
    m_container[eng->getID()] = eng; 
    return ret;
}

Int32 CmdCtx::downloadFile(EvMsgStartDownload* msg) {
    Int32 ret = 0;
    Engine* eng = NULL;
    const Char* id = NULL;
    typeTabItr itr;

    id = MsgCenter::ID(msg); 
    
    itr = m_container.find(id);
    if (m_container.end() == itr) {
        I_NEW_1(ReceiverEngine, eng, this);
        ret = eng->start();
        if (0 != ret) {
            return ret;
        }

        eng->setID(id);
        eng->notify(msg);
        m_container[eng->getID()] = eng; 

        return 0;
    } else {
        /* the task is already running, cannot setup again */
        return -1;
    }
}

Int32 CmdCtx::acceptDownload(Void* msg) {
    Int32 ret = 0;
    Engine* eng = NULL;
    const Char* id = NULL;

    id = MsgCenter::ID(msg); 
    I_NEW_1(SenderEngine, eng, this);
    ret = eng->start();
    if (0 != ret) {
        return ret;
    }
    
    eng->setID(id);
    eng->notify(msg); 
    m_container[eng->getID()] = eng; 
    return ret;
}

Int32 CmdCtx::recvPkg(Void* ) { 
    return 0; 
}

Int32 CmdCtx::sendPkg(Void* msg) {
    if (NULL != m_test) {
        m_test->notify(msg); 
    }

    return 0; 
}

Int32 CmdCtx::transPkg(Void* msg) {
    Int32 ret = 0;

    ret = sendPkg(msg);
    return ret;
}

Void CmdCtx::dealEvent(Void* msg) {
    Int32 cmd = 0;
    const Char* id = NULL;
    Engine* eng = NULL;
    typeTabItr itr;

    id = MsgCenter::ID(msg);
    cmd = MsgCenter::cmd(msg);

    if (CMD_REQ_FILE_UPLOAD == cmd) {
        itr = m_container.find(id);
        if (itr == m_container.end()) {
            acceptUpload(msg);
        } else {
            /* invalid req */
            MsgCenter::freeMsg(msg);
        }
    } else if (CMD_REQ_FILE_DOWNLOAD == cmd) {
        itr = m_container.find(id);
        if (itr == m_container.end()) {
            acceptDownload(msg);
        } else {
            /* invalid req */
            MsgCenter::freeMsg(msg);
        }
    } else {
        itr = m_container.find(id);
        if (itr != m_container.end()) {
            eng = itr->second;
            eng->notify(msg);
        } else {
            MsgCenter::freeMsg(msg);
        }
    } 
} 

Void CmdCtx::test_01() {
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
        
        uploadFile(pMsg);
    }
}

Void CmdCtx::test_02() {
    char fileId[] = "task_id_123456";
    char taskId[] = "test_02_task";
    EvMsgStartDownload* pMsg = NULL;

    pMsg = MsgCenter::creatMsg<EvMsgStartDownload>(CMD_START_FILE_DOWNLOAD);
    
    strncpy(pMsg->m_file_id, fileId, sizeof(pMsg->m_file_id));
    
    MsgCenter::setID(taskId, pMsg);
    
    downloadFile(pMsg);
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




