#include"taskstat.h"
#include"msgcenter.h"
#include"ictx.h"


TaskFailEnd::TaskFailEnd() {
}

TaskFailEnd::~TaskFailEnd() {
}

Void TaskFailEnd::prepare(I_Ctx* ctx) {        
    LOG_INFO("===========TaskFailEnd|==");

    ctx->reportResult();
}

Void TaskFailEnd::process(Void* msg) {
    LOG_INFO("===========TaskFailEnd::process==");

    MsgCenter::freeMsg(msg);
}

Void TaskFailEnd::post() {
}

TaskSucessEnd::TaskSucessEnd() {
}

TaskSucessEnd::~TaskSucessEnd() {
}

Void TaskSucessEnd::prepare(I_Ctx* ctx) {
    
    LOG_INFO("===========TaskSucessEnd::prepare==");

    ctx->reportResult();
}

Void TaskSucessEnd::process(Void* msg) {
    LOG_INFO("===========TaskSucessEnd::process==");

    MsgCenter::freeMsg(msg);
}

Void TaskSucessEnd::post() {
} 


