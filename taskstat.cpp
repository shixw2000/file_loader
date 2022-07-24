#include<string.h>
#include"taskstat.h"
#include"filetool.h"
#include"msgcenter.h"
#include"ievent.h"
#include"filectx.h"
#include"eventmsg.h"
#include"engine.h"


TaskFailEnd::TaskFailEnd() {
}

TaskFailEnd::~TaskFailEnd() {
}

Void TaskFailEnd::prepare(I_Ctx*) {    
    LOG_INFO("===========TaskFailEnd|==");
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

Void TaskSucessEnd::prepare(I_Ctx*) {
    LOG_INFO("===========TaskSucessEnd::prepare==");
}

Void TaskSucessEnd::process(Void* msg) {
    LOG_INFO("===========TaskSucessEnd::process==");

    MsgCenter::freeMsg(msg);
}

Void TaskSucessEnd::post() {
}

