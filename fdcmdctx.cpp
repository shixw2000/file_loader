#include"fdcmdctx.h"
#include"filetool.h"
#include"engine.h"


FdCmdCtx::FdCmdCtx() {
    m_has_event = FALSE;
}

FdCmdCtx::~FdCmdCtx() {
}

Int32 FdCmdCtx::start() {
    Int32 ret = 0;
    Int32 fd = 0;

    ret = CmdCtx::start();
    if (0 != ret) {
        return ret;
    }
    
    fd = creatEventFd();
    if (0 <= fd) {
        setFd(fd);
        return 0;
    } else {
        return -1;
    } 
}

Void FdCmdCtx::stop() {
    FileTool::closeFd(&m_fd);

    CmdCtx::stop();
}

Void FdCmdCtx::read(Int32 fd) {
    Int32 ret = 0;

    ret = readEvent(fd);
    if (0 == ret) {
        m_has_event = FALSE;
        
        run();
    }
}

Bool FdCmdCtx::notify(Void* msg) {
    Bool bOk = TRUE;
    
    /* may use lock here */
    bOk = QueWorker::notify(msg); 
    if (bOk && !m_has_event) {
        writeEvent(getFd());

        m_has_event = TRUE;
    }

    return bOk;
}

Bool FdCmdCtx::emerge(Void* msg) {
    Bool bOk = TRUE;
    
    /* may use lock here */
    bOk = QueWorker::emerge(msg); 

    if (bOk && !m_has_event) {
        writeEvent(getFd());

        m_has_event = TRUE;
    }

    return bOk;
}

/* add a engine, and trigger a event if not running */
Void FdCmdCtx::addRunList(Engine* eng) {
    if (ENUM_ENG_IDLE == eng->status()) {
        m_run_list.push_back(eng);
        eng->setStatus(ENUM_ENG_RUNNING);

        if (!m_has_event) {
            writeEvent(getFd());
            m_has_event = TRUE;
        }
    }
}

Void FdCmdCtx::delRunList(Engine* eng) {
    if (ENUM_ENG_RUNNING == eng->status()) {
        m_run_list.remove(eng);
        eng->setStatus(ENUM_ENG_IDLE);
    }
}

Void FdCmdCtx::run() {    
    consume(); 
    doTasks();
} 

Void FdCmdCtx::doTasks() {
    Engine* eng = NULL;
    
    while (!m_run_list.empty()) {
        eng = m_run_list.front();
        m_run_list.pop_front();
        
        eng->setStatus(ENUM_ENG_IDLE);

        eng->consume();
    }
}

