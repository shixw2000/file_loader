#include<sys/types.h>
#include<unistd.h>
#include<time.h>
#include"ievent.h"
#include"msgcenter.h"


struct MsgCenter::_internal {
    list_node m_node;
    Int32 m_total;
    Int32 m_pos;
};

Int32 MsgCenter::cmd(const Void* msg) {
    const EvMsgHeader* hd = NULL;

    hd = msg2head(msg);
    return hd->m_ev;
}

Int32 MsgCenter::error(const Void* msg) {
    const EvMsgHeader* hd = NULL;

    hd = msg2head(msg);
    return hd->m_code;
}

const Char* MsgCenter::ID(const Void* msg) {
    const EvMsgHeader* hd = NULL;

    hd = msg2head(msg);
    return hd->m_task_id;
}

Void MsgCenter::setError(Int32 err, Void* msg) {
    EvMsgHeader* hd = NULL;

    hd = msg2head(msg);
    
    hd->m_code = err;
}

Void MsgCenter::setID(const Char ID[], Void* msg) {
    EvMsgHeader* hd = NULL;

    hd = msg2head(msg);

    strncpy(hd->m_task_id, ID, MAX_TASKID_SIZE);
}

void MsgCenter::notify(Void* msg, list_head* head) {
    list_node* node = NULL;

    node = msg2node(msg); 
    list_add_back(node, head); 
}

void MsgCenter::emerge(Void* msg, list_head* head) {
    list_node* node = NULL;

    node = msg2node(msg); 
    list_add_front(node, head);
}

void MsgCenter::add(Void* msg, order_list_head* head) {
    list_node* node = NULL;

    node = msg2node(msg); 
    
    order_list_add(node, head); 
}

Void* MsgCenter::prepend(Int32 len) {
    Int32 size = 0;
    _internal* intern = NULL;
    Char* psz = NULL;

    if (DEF_MSG_HEADER_SIZE <= len) {
        size = (Int32)sizeof(_internal) + len;
        ARR_NEW(Char, size, psz);
        if (NULL != psz) {
            memset(psz, 0, size);

            intern = (_internal*)psz; 
            INIT_LIST_NODE(&intern->m_node);
            intern->m_total = len;
            intern->m_pos = 0;

            /* step to msg body */
            psz += sizeof(_internal) + DEF_MSG_HEADER_SIZE;
            return psz;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

Void* MsgCenter::dupUdp(const Void* buf, Int32 len) {
    Void* msg = NULL;
    const EvMsgHeader* hd = NULL;

    hd = (const EvMsgHeader*)buf;
    if (DEF_MSG_HEADER_SIZE <= len && hd->m_size == len) { 
    
        msg = prepend(len);
        if (NULL != msg) {
            memcpy(buffer(msg), buf, len);

            return msg;
        } else {
            return NULL;
        }
    } else {
        LOG_INFO("dupUdp| rdlen=%d| msg_size=%d| error=invalid msg|",
            len, hd->m_size);
        
        return NULL;
    }
}

Int32 MsgCenter::fillMsg(const Void* buf, Int32 len, Void* msg) {
    Int32 left = 0;
    Char* psz = NULL;
    _internal* intern = NULL;

    intern = msg2intern(msg);
    psz = buffer(msg);
    psz += intern->m_pos;

    left = intern->m_total - intern->m_pos;
    if (left <= len) {
        /* read all */
        memcpy(psz, buf, left);
        
        intern->m_pos = intern->m_total;
        return left;
    } else {
        /* read partial */
        memcpy(psz, buf, len);
        intern->m_pos += len;

        return len;
    } 
}

Bool MsgCenter::endOfMsg(const Void* msg) {
    const _internal* intern = NULL;

    intern = msg2intern(msg);
    return intern->m_total == intern->m_pos;
}

Void MsgCenter::reopen(Void* msg) {
    _internal* intern = NULL;

    intern = msg2intern(msg);
    intern->m_pos = 0;
}

Void* MsgCenter::creat(Int32 type, Int32 extlen) {
    _internal* intern = NULL;
    EvMsgHeader* hd = NULL;
    Char* buf = NULL;
    Int32 msglen = 0;
    Int32 size = 0;

    msglen = (Int32)sizeof(EvMsgHeader) + extlen;
    size = (Int32)sizeof(_internal) + msglen;
    ARR_NEW(Char, size, buf);
    
    if (NULL != buf) {
        memset(buf, 0, size);
        
        intern = (_internal*)buf; 
        INIT_LIST_NODE(&intern->m_node);
        intern->m_total = msglen;
        intern->m_pos = 0;

        hd = (EvMsgHeader*)(buf + sizeof(_internal));
        hd->m_size = msglen;
        hd->m_ev = type;

        buf += sizeof(_internal) + sizeof(EvMsgHeader);
        return buf;
    } else {
        return NULL;
    }
}

list_node* MsgCenter::msg2node(Void* msg) {
    _internal* intern = NULL;

    intern = msg2intern(msg); 
    return &intern->m_node;
}

Void* MsgCenter::node2msg(list_node* node) {
    _internal* intern = NULL;
    Char* buf = NULL;
    
    intern = list_entry(node, _internal, m_node);
    buf = reinterpret_cast<Char*>(intern);
    
    buf += sizeof(_internal) + sizeof(EvMsgHeader); 
    return buf;
}

void MsgCenter::freeMsg(Void* msg) { 
    if (NULL != msg) {
        Char* buf = (Char*)msg;
        
        buf -= sizeof(_internal) + sizeof(EvMsgHeader); 
        ARR_FREE(buf);
    }
}

Char* MsgCenter::buffer(Void* msg) {
    EvMsgHeader* hd = NULL;

    hd = msg2head(msg);
    return (Char*)hd;
}

Int32 MsgCenter::bufsize(Void* msg) {
    _internal* intern = NULL;

    intern = msg2intern(msg);
    return intern->m_total;
}

Int32 MsgCenter::bufpos(Void* msg) {
    _internal* intern = NULL;

    intern = msg2intern(msg);
    return intern->m_pos;
}

Void MsgCenter::setbufpos(Int32 pos, Void* msg) {
    _internal* intern = NULL;

    intern = msg2intern(msg);
    intern->m_pos = pos;
}

MsgCenter::_internal* MsgCenter::msg2intern(Void* msg) {
    _internal* intern = NULL;
    Char* buf = reinterpret_cast<Char*>(msg);
    
    buf -= sizeof(_internal) + sizeof(EvMsgHeader);
    intern = reinterpret_cast<_internal*>(buf);

    return intern;
}

const MsgCenter::_internal* MsgCenter::msg2intern(
    const Void* msg) {
    const _internal* intern = NULL;
    const Char* buf = reinterpret_cast<const Char*>(msg);
    
    buf -= sizeof(_internal) + sizeof(EvMsgHeader);
    intern = reinterpret_cast<const _internal*>(buf);

    return intern;
}

EvMsgHeader* MsgCenter::msg2head(Void* msg) {
    EvMsgHeader* hd = NULL;
    Char* buf = reinterpret_cast<Char*>(msg);

    buf -= sizeof(EvMsgHeader);
    hd = reinterpret_cast<EvMsgHeader*>(buf);

    return hd;
}

const EvMsgHeader* MsgCenter::msg2head(const Void* msg) {
    const EvMsgHeader* hd = NULL;
    const Char* buf = reinterpret_cast<const Char*>(msg);
    
    buf -= sizeof(EvMsgHeader);
    hd = reinterpret_cast<const EvMsgHeader*>(buf);

    return hd;
}




