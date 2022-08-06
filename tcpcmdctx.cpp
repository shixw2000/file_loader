#include<string.h>
#include"tcpcmdctx.h"
#include"cmdctx.h"
#include"eventoper.h"
#include"sockutil.h"
#include"filepoll.h"
#include"msgcenter.h"


char TcpBaseOper::m_tcp_buf[DEF_TCP_RECV_BUF_SIZE];

TcpBaseOper::TcpBaseOper() {
    m_handler = NULL;
    m_snd_msg = NULL;
    m_rcv_msg = NULL;
    
    m_header_len = 0;
}

TcpBaseOper::~TcpBaseOper() {
}

Void TcpBaseOper::setParam(I_Handler* handler) {
    m_handler = handler;
}

Void TcpBaseOper::finish() {
    if (NULL != m_snd_msg) {
        MsgCenter::freeMsg(m_snd_msg);
    }

    if (NULL != m_rcv_msg) {
        MsgCenter::freeMsg(m_rcv_msg);
    }
}


/* return: 0: ok, -1: error */
Int32 TcpBaseOper::readFd(PollItem* item) {
    Int32 rdlen = 0;
    Int32 used = 0;
    Int32 left = 0;
    const Char* psz = NULL;
    
    while (TRUE) {
        rdlen = recvTcp(item->m_fd, m_tcp_buf, DEF_TCP_RECV_BUF_SIZE);
        if (0 < rdlen) {
            psz = m_tcp_buf;
            left = rdlen;
            while (0 < left) {
                used = parse(psz, left, item);
                if (0 <= used) {
                    psz += used;
                    left -= used; 
                } else {
                    LOG_INFO("readFd| fd=%d| rdlen=%d| left=%d| used=%d|"
                        " msg=parse error|",
                        item->m_fd, rdlen, left, used);
                    return -1;
                } 
            }

            continue;
        } else if (0 == rdlen) {
            return 0;
        } else {
            LOG_INFO("readFd| fd=%d| rdlen=%d| msg=recvTcp error|",
                item->m_fd, rdlen);
            return -1;
        }
    }
} 

Bool TcpBaseOper::chkHeader(const EvMsgHeader* hd) {
    if (DEF_MSG_HEADER_SIZE <= hd->m_size 
        && DEF_MAX_TCP_FRAME_SIZE >= hd->m_size) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Int32 TcpBaseOper::parse(const Char* buf, int len, PollItem* item) {
    Int32 left = 0;
    Int32 used = 0;
    Bool bOk = TRUE;
    const EvMsgHeader* hd = NULL;

    if (0 == m_header_len && DEF_MSG_HEADER_SIZE <= len) {
        hd = (const EvMsgHeader*)buf;
        bOk = chkHeader(hd);
        if (bOk) {
            m_header_len = DEF_MSG_HEADER_SIZE;
            m_rcv_msg = MsgCenter::prepend(hd->m_size);
            if (NULL != m_rcv_msg) {
                used = fillMsg(buf, len, item);
                return used;
            } else {
                return -1;
            }
        } else {
            return -1;
        }
    } else if (m_header_len < DEF_MSG_HEADER_SIZE) {
        left = DEF_MSG_HEADER_SIZE - m_header_len;
        if (left <= len) {
            memcpy(&m_header[m_header_len], buf, left);
            
            m_header_len += left;
            buf += left;
            len -= left;

            hd = (const EvMsgHeader*)m_header;
            bOk = chkHeader(hd);
            if (bOk) {
                m_rcv_msg = MsgCenter::prepend(hd->m_size);

                /* fill header */
                MsgCenter::fillMsg(hd, DEF_MSG_HEADER_SIZE, m_rcv_msg);

                /* file body */
                used = fillMsg(buf, len, item);
                return left + used;
            } else {
                return -1;
            }
        } else {
            memcpy(&m_header[m_header_len], buf, len);
            m_header_len += len;

            return len;
        }
    } else {
        /* fullfil the msg */
        used = fillMsg(buf, len, item);
        return used;
    }
}
    
Int32 TcpBaseOper::fillMsg(const Char* buf, int len, PollItem* item) {
    Int32 used = 0;
    Bool eom = FALSE;

    used = MsgCenter::fillMsg(buf, len, m_rcv_msg);
    eom = MsgCenter::endOfMsg(m_rcv_msg);
    if (eom) {
        MsgCenter::reopen(m_rcv_msg);
        MsgCenter::notify(m_rcv_msg, &item->m_rcv_msgs);

        m_rcv_msg = NULL;
        m_header_len = 0;
    }

    return used;
}

/* return: 0: ok, -1: error */
Int32 TcpBaseOper::writeFd(PollItem* item) {
    Int32 size = 0;
    Int32 used = 0;
    Int32 left = 0;
    Int32 sndlen = 0;
    list_node* pos = NULL;
    const Char* psz = NULL;

    while (TRUE) {
        if (NULL == m_snd_msg) {
            if (!list_empty(&item->m_snd_msgs)) {
                pos = LIST_FIRST(&item->m_snd_msgs);
                list_del(pos, &item->m_snd_msgs);
                
                m_snd_msg = MsgCenter::entry<Void>(pos);
            } else {
                item->m_more_write = FALSE;
                return 0;
            }
        }

        psz = MsgCenter::buffer(m_snd_msg);
        size = MsgCenter::bufsize(m_snd_msg);
        used = MsgCenter::bufpos(m_snd_msg);

        psz += used;
        left = size - used;
        sndlen = sendTcp(item->m_fd, psz, left);
        
        LOG_DEBUG("writeFd| fd=%d| cmd=%d| size=%d| used=%d|"
            " left=%d| sndlen=%d| msg=sendTcp|",
            item->m_fd, MsgCenter::cmd(m_snd_msg), 
            size, used, left, sndlen);
        
        if (0 < sndlen) {
            if (sndlen == left) {
                MsgCenter::freeMsg(m_snd_msg);
                m_snd_msg = NULL;
            } else {
                /* just part of msg sent */
                used += sndlen;
                MsgCenter::setbufpos(used, m_snd_msg);
            }

            continue;
        } else if (0 == sndlen) {
            /* busy for send, notify it */
            item->m_can_write = FALSE;
            return 0;
        } else {
            LOG_INFO("***writeFd| fd=%d| cmd=%d| size=%d| used=%d|"
                " left=%d| sndlen=%d| msg=sendTcp error|",
                item->m_fd, MsgCenter::cmd(m_snd_msg), 
                size, used, left, sndlen);
            
            return -1;
        }
    }
}

Int32 TcpBaseOper::dealFd(PollItem* item) {
    Int32 ret = 0;
    list_node* pos = NULL;
    list_node* n = NULL;
    Void* msg = NULL;

    list_for_each_safe(pos, n, &item->m_rcv_msgs) {
        list_del(pos, &item->m_rcv_msgs);
                
        msg = MsgCenter::entry<Void>(pos);
        m_handler->process(msg);
    }
    
    return ret; 
}


TcpCmdCtx::TcpCmdCtx() {
    m_poll = NULL;
    m_ctx = NULL;
    
    m_ev_oper = NULL;
    m_tcp_oper = NULL;
    m_tcp_fd = -1;
    m_ev_fd = -1;
    m_end = FALSE;
}

TcpCmdCtx::~TcpCmdCtx() {
}

Void TcpCmdCtx::setParam(Int32 fd, FilePoll* poll, CmdCtx* ctx) { 
    m_poll = poll;
    m_ctx = ctx;
    m_tcp_fd = fd; 
}

Int32 TcpCmdCtx::init() {    
    Int32 ret = 0; 

    m_ev_fd = creatEventFd();
    if (0 > m_ev_fd) {
        return -1;
    }

    I_NEW(TcpBaseOper, m_tcp_oper);
    m_tcp_oper->setParam(this);
    
    I_NEW(TcpSignalOper, m_ev_oper);
    m_ev_oper->setParam(m_ev_fd, this); 

    ret = m_poll->addOper(m_ev_fd, m_ev_oper, FALSE, TRUE, FALSE);
    if (0 != ret) {
        return -1;
    }
    
    return ret;
}

Void TcpCmdCtx::finish() { 
    if (0 <= m_ev_fd) {
        m_poll->delOper(m_ev_fd);

        closeHd(m_ev_fd);
        m_ev_fd = -1;
    }
  
    if (NULL != m_ev_oper) {
        m_ev_oper->finish();
        I_FREE(m_ev_oper);
    }

    if (NULL != m_tcp_oper) {
        m_tcp_oper->finish();
        I_FREE(m_tcp_oper);
    }

    /* ctx release here, though it is not alloc by it */
    if (NULL != m_ctx) {
        m_ctx->stop();
        I_FREE(m_ctx);
    }
}

Int32 TcpCmdCtx::sendPkg(Void* msg) {
    Int32 ret = 0;

    ret = m_poll->addSendQue(m_tcp_fd, msg);
    return ret;
}

Int32 TcpCmdCtx::transPkg(Void* msg) {
    return sendPkg(msg);
}

Void TcpCmdCtx::event(Uint32 val) {
    LOG_DEBUG("process_event| val=%u| now=%u|", val, m_ctx->now());
    
    m_ctx->run();
}

Void TcpCmdCtx::process(Void* msg) {
    LOG_DEBUG("process_msg| cmd=%d| len=%d| now=%u",
        MsgCenter::cmd(msg), MsgCenter::bufsize(msg), m_ctx->now());
    
    m_ctx->recvPkg(msg);
}

Void TcpCmdCtx::signal() {
    m_ev_oper->signal();
}

Void TcpCmdCtx::reportResult(const ReportFileInfo* info) {
    LOG_INFO("report_result| result=%d| file_name=%s| size=%lld|"
        " upload_download=%d| send_recv=%d|"
        " file_id=%s| task_id=%s|",
        info->m_result, info->m_file_name, info->m_file_size,
        info->m_upload_download, info->m_send_recv,
        info->m_file_id, info->m_task_id);
}

Int32 TcpCmdCtx::readFd(PollItem* item) {
    Int32 ret = 0;

    if (!eof()) {
        ret = m_tcp_oper->readFd(item);
    } else {
        ret = -1;
    }
    
    return ret;
}

Int32 TcpCmdCtx::writeFd(PollItem* item) {
    Int32 ret = 0;

    if (!eof()) {
        ret = m_tcp_oper->writeFd(item);
    } else {
        ret = -1;
    }

    return ret;
}

Int32 TcpCmdCtx::dealFd(PollItem* item) {
    Int32 ret = 0;

    if (!eof()) {
        ret = m_tcp_oper->dealFd(item);
    } else {
        ret = -1;
    }

    return ret;
}


TcpSignalOper::TcpSignalOper() {
    m_cb = NULL;
}

TcpSignalOper::~TcpSignalOper() {
}

Void TcpSignalOper::setParam(Int32 fd, TcpCmdCtx* cb) {
    m_cb = cb;
    SignalOper::setParam(fd, m_cb);
}

Void TcpSignalOper::finish() {        
    if (NULL != m_cb) {
        m_cb->setEof();
    }
}

