#include<sys/types.h>
#include<unistd.h>
#include<time.h>
#include"ievent.h"
#include"msgcenter.h"


Uint32 MsgCenter::random() {
    Uint32 n = 0;

    n = getpid();
    return n;
}

Uint64 MsgCenter::clock() {
    Uint64 n = 0;
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);
    n = tp.tv_sec * 1000000 + tp.tv_nsec/1000;
    return n;
}


Int32 MsgCenter::cmd(const Void* msg) {
    const Char* psz = (const Char*)msg;
    const EvMsgHeader* pHd = NULL;

    psz -= sizeof(EvMsgHeader);
    pHd = (const EvMsgHeader*)psz;
    return pHd->m_ev;
}

Int32 MsgCenter::error(const Void* msg) {
    const Char* psz = (const Char*)msg;
    const EvMsgHeader* pHd = NULL;

    psz -= sizeof(EvMsgHeader);
    pHd = (const EvMsgHeader*)psz;
    return pHd->m_code;
}

const Char* MsgCenter::ID(const Void* msg) {
    const Char* psz = (const Char*)msg;
    const EvMsgHeader* pHd = NULL;

    psz -= sizeof(EvMsgHeader);
    pHd = (const EvMsgHeader*)psz;
    return pHd->m_task_id;
}

Void MsgCenter::setError(Int32 err, Void* msg) {
    Char* psz = (Char*)msg;
    EvMsgHeader* pHd = NULL;

    psz -= sizeof(EvMsgHeader);
    pHd = (EvMsgHeader*)psz;
    pHd->m_code = err;
}

Void MsgCenter::setID(const Char ID[], Void* msg) {
    Char* psz = (Char*)msg;
    EvMsgHeader* pHd = NULL;

    psz -= sizeof(EvMsgHeader);
    pHd = (EvMsgHeader*)psz;

    strncpy(pHd->m_task_id, ID, MAX_TASKID_SIZE);
}

void MsgCenter::notify(Void* msg, list_head* head) {
    list_node* node = NULL;

    node = to(msg); 
    list_add_back(node, head); 
}

void MsgCenter::emerge(Void* msg, list_head* head) {
    list_node* node = NULL;

    node = to(msg); 
    list_add_front(node, head);
}

void MsgCenter::add(Void* msg, order_list_head* head) {
    list_node* node = NULL;

    node = to(msg); 
    
    order_list_add(node, head); 
}

Void* MsgCenter::creat(Int32 type, Int32 extlen) {
    list_node* node = NULL;
    EvMsgHeader* hd = NULL;
    Char* buf = NULL;
    Int32 msglen = 0;
    Int32 size = 0;

    msglen = (Int32)sizeof(EvMsgHeader) + extlen;
    size = (Int32)sizeof(list_node) + msglen;
    ARR_NEW(Char, size, buf);
    
    if (NULL != buf) {
        memset(buf, 0, size);
        
        node = (list_node*)buf; 
        INIT_LIST_NODE(node);

        hd = (EvMsgHeader*)(buf + sizeof(list_node));
        hd->m_size = msglen;
        hd->m_ev = type;

        buf += sizeof(list_node) + sizeof(EvMsgHeader);
        return buf;
    } else {
        return NULL;
    }
}

list_node* MsgCenter::to(Void* msg) {
    Char* buf = reinterpret_cast<Char*>(msg);
    
    if (NULL != buf) { 
        buf -= sizeof(list_node) + sizeof(EvMsgHeader); 
    }

    return reinterpret_cast<list_node*>(buf);
}

Void* MsgCenter::from(list_node* node) {
    Char* buf = reinterpret_cast<Char*>(node);
    
    if (NULL != buf) { 
        buf += sizeof(list_node) + sizeof(EvMsgHeader); 
    }

    return buf;
}

void MsgCenter::freeMsg(Void* msg) { 
    if (NULL != msg) {
        Char* buf = (Char*)msg;
        
        buf -= sizeof(list_node) + sizeof(EvMsgHeader); 
        ARR_FREE(buf);
    }
}

