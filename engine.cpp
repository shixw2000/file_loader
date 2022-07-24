#include<string.h>
#include"engine.h"
#include"msgcenter.h"
#include"cmdctx.h"
#include"ictx.h"


Engine::Engine(CmdCtx* env) {
    m_env = env;
    
    m_data = NULL;
    m_ctx = NULL; 
    m_status = ENUM_ENG_IDLE;
}

Engine::~Engine() { 
}

Int32 Engine::start() {
    Int32 ret = 0;

    I_NEW(TransBaseType, m_data);
    resetTransData(m_data);

    m_ctx = creatCtx();
    ret = m_ctx->start();
    if (0 != ret) {
        return ret;
    }

    ret = QueWorker::start();
    
    return ret;
}

Void Engine::stop() {
    QueWorker::stop();
    QueWorker::consume();
    
    if (NULL != m_ctx) {
        m_ctx->stop();

        freeCtx(m_ctx);
        m_ctx = NULL;
    }
    
    I_FREE(m_data);
}

Void Engine::dealEvent(Void* msg) {
    m_ctx->procEvent(msg);
}

Void Engine::setID(const Char* taskid) { 
    strncpy(m_data->m_task_id, taskid, MAX_TASKID_SIZE);
}

const Char* Engine::getID() const {
    return m_data->m_task_id;
}

Uint32 Engine::now() const {
    return m_env->now();
}

TransBaseType* Engine::getData() const {
    return m_data;
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

Void* Engine::creatTimer(Int32 type) {
    return m_env->creatTimer(this, type);
}

Void Engine::delTimer(Void* id) {
    m_env->delTimer(id);
}

Void Engine::resetTimer(Void* id, Uint32 tick) {
    m_env->resetTimer(id, tick);
}

Void Engine::startTimer(Void* id, Uint32 tick) {
    m_env->startTimer(id, tick);
}

Void Engine::stopTimer(Void* id) {
    m_env->stopTimer(id);
}

bool Engine::operator<(const Engine* o) const {
    return 0 > strcmp(m_data->m_task_id, o->m_data->m_task_id);
}

bool Engine::operator==(const Engine* o) const {
    return 0 == strcmp(m_data->m_task_id, o->m_data->m_task_id);
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

