#include<string.h>
#include"tcpoper.h"
#include"sockutil.h"
#include"filepoll.h"
#include"msgcenter.h"
#include"tcpcmdctx.h"
#include"ticktimer.h"
#include"timeroper.h"
#include"cmdctx.h"


/* return: 0: ok, -1: error */
Int32 TcpSrvOper::readFd(PollItem* item) {
    Int32 ret = 0;
    Int32 newFd = 0;
    
    while (TRUE) {
        newFd = acceptCli(item->m_fd);
        if (0 <= newFd) {
            ret = addCliOper(newFd);
            if (0 == ret) { 
                LOG_INFO("accept_sock| listener=%d| new_sock=%d| msg=ok|",
                    item->m_fd, newFd);
            } else {
                LOG_INFO("accept_sock| listener=%d| new_sock=%d| error=%d|",
                    item->m_fd, newFd, ret);
            }
        } else if (-2 == newFd) {
            return 0;
        } else {
            return -1;
        }
    }
} 


TcpFileSrv::TcpFileSrv() {
    m_thr = NULL;
    m_poll = NULL;
    m_timer = NULL;
    m_timer_oper = NULL;
    m_timer_fd = -1;
    m_listen_fd = -1;
    m_port = 0;

    m_conf_send_speed = 0;
    m_conf_recv_speed = 0;
}

TcpFileSrv::~TcpFileSrv() {
    if (NULL != m_poll) {
        m_poll->finish();
        I_FREE(m_poll);
    }
    
    I_FREE(m_thr);
}

Int32 TcpFileSrv::init() {
    Int32 ret = 0;

    m_listen_fd = creatTcpSrv(m_ip, m_port);
    if (0 > m_listen_fd) {
        return -1;
    }

    m_timer_fd = creatTimerFd(DEF_TICK_TIME_MS);
    if (0 > m_timer_fd) {
        return -1;
    }

    I_NEW(FilePoll, m_poll);
    ret = m_poll->init();
    if (0 != ret) {
        return -1;
    }

    I_NEW(TickTimer, m_timer);
    ret = m_timer->start();
    if (0 != ret) {
        return -1;
    }

    I_NEW(TickTimer, m_timer);
    ret = m_timer->start();
    if (0 != ret) {
        return -1;
    }

    I_NEW(TimerOper, m_timer_oper);
    m_timer_oper->setParam(m_timer);

    ret = m_poll->addOper(m_timer_fd, m_timer_oper, FALSE, TRUE, FALSE);
    if (0 != ret) {
        return -1;
    }

    ret = m_poll->addOper(m_listen_fd, this, FALSE, TRUE, FALSE);
    if (0 != ret) {
        return -1;
    }

    I_NEW(CThread, m_thr);
    m_thr->setParam(this);

    return 0;
}

Void TcpFileSrv::finish() {
    if (NULL != m_thr) {
        m_thr->stop();
    }
    
    if (0 <= m_timer_fd) {
        m_poll->delOper(m_timer_fd);
        m_timer_fd = -1;
    }

    if (0 <= m_listen_fd) {
        m_poll->delOper(m_listen_fd);
        m_listen_fd = -1;
    }

    if (NULL != m_timer_oper) {
        m_timer_oper->finish();
        I_FREE(m_timer_oper);
    }
    
    if (NULL != m_timer) {
        m_timer->stop();
        I_FREE(m_timer);
    } 
}

Int32 TcpFileSrv::start() {
    Int32 ret = 0;

    ret = m_thr->start();
    return ret;
}

Void TcpFileSrv::stop() {
    m_thr->stop();
}

Int32 TcpFileSrv::run() {
    Int32 ret = 0;
    
    ret = m_poll->run();
    return ret;
}

Void TcpFileSrv::setParam(const Char ip[], int port) { 
    m_port = port;
    strncpy(m_ip, ip, DEF_IP_SIZE);
}

Void TcpFileSrv::setConfSpeed(Int32 sndSpeed, Int32 rcvSpeed) {
    m_conf_send_speed = sndSpeed;
    m_conf_recv_speed = rcvSpeed;
}

Int32 TcpFileSrv::addCliOper(Int32 fd) {
    Int32 ret = 0;
    TcpCmdCtx* cliOper = NULL;
    CmdCtx* ctx = NULL;

    I_NEW(TcpCmdCtx, cliOper);
    I_NEW(CmdCtx, ctx);

    cliOper->setParam(fd, m_poll, ctx);
    ctx->setParam(m_timer, cliOper); 

    do { 
        ret = cliOper->init();
        if (0 != ret) {
            break;
        } 
        
        ret = ctx->start();
        if (0 != ret) {
            break;
        }

        ctx->setSpeed(m_conf_send_speed, m_conf_recv_speed); 
        
        ret = m_poll->addOper(fd, cliOper, TRUE, TRUE, TRUE);
        if (0 == ret) {
            return 0;
        } else {
            return -1;
        } 
    } while (0);

    cliOper->finish();
    I_FREE(cliOper);
    return ret;
}


TcpFileCli::TcpFileCli() {
    m_thr = NULL;
    m_poll = NULL;
    m_timer = NULL;
    m_ctx = NULL;
    m_timer_oper = NULL;
    m_timer_fd = -1;
    m_cli_fd = -1;
    m_port = 0;
}

TcpFileCli::~TcpFileCli() {
    if (NULL != m_poll) {
        m_poll->finish();
        I_FREE(m_poll);
    }
    
    I_FREE(m_thr);
}

Int32 TcpFileCli::init() {
    Int32 ret = 0;

    I_NEW(CThread, m_thr);
    m_thr->setParam(this);

    m_cli_fd = creatTcpCli(m_ip, m_port);
    if (0 > m_cli_fd) {
        return -1;
    }

    m_timer_fd = creatTimerFd(DEF_TICK_TIME_MS);
    if (0 > m_timer_fd) {
        return -1;
    }

    I_NEW(FilePoll, m_poll);
    ret = m_poll->init();
    if (0 != ret) {
        return -1;
    }

    I_NEW(TickTimer, m_timer);
    ret = m_timer->start();
    if (0 != ret) {
        return -1;
    }

    I_NEW(TimerOper, m_timer_oper);
    m_timer_oper->setParam(m_timer);

    I_NEW(CmdCtx, m_ctx);
    m_ctx->setParam(m_timer, this); 

    /* if create, must first attach to tcpCmdCtx for ctx */
    TcpCmdCtx::setParam(m_cli_fd, m_poll, m_ctx);
    ret = TcpCmdCtx::init();
    if (0 != ret) {
        return -1;
    }

    ret = m_ctx->start();
    if (0 != ret) {
        return -1;
    }

    ret = m_poll->addOper(m_timer_fd, m_timer_oper, FALSE, TRUE, FALSE);
    if (0 != ret) {
        return -1;
    }

    ret = m_poll->addOper(m_cli_fd, this, FALSE, TRUE, TRUE);
    if (0 != ret) {
        return -1;
    }

    return 0;
}

Void TcpFileCli::finish() { 
    if (NULL != m_thr) {
        m_thr->stop();
    }

    TcpCmdCtx::finish();
    
    if (0 <= m_timer_fd) {
        m_poll->delOper(m_timer_fd);
        m_timer_fd = -1;
    }

    if (0 <= m_cli_fd) {
        m_poll->delOper(m_cli_fd);
        m_cli_fd = -1;
    }

    if (NULL != m_timer_oper) {
        m_timer_oper->finish();
        I_FREE(m_timer_oper);
    }
    
    if (NULL != m_timer) {
        m_timer->stop();
        I_FREE(m_timer);
    }

    if (NULL != m_ctx) {
        /* ctx is released by tcpcmdctx !! */
        m_ctx = NULL;
    }
}

Int32 TcpFileCli::start() {
    Int32 ret = 0; 

    ret = m_thr->start();
    return ret;
}

Void TcpFileCli::stop() {
    m_thr->stop();
}

Int32 TcpFileCli::run() {
    Int32 ret = 0;
    
    ret = m_poll->run();
    return ret;
}

Void TcpFileCli::setParam(const Char ip[], int port) { 
    m_port = port;
    strncpy(m_ip, ip, DEF_IP_SIZE);
}

Void TcpFileCli::setConfSpeed(Int32 sndSpeed, Int32 rcvSpeed) { 
    m_ctx->setSpeed(sndSpeed, rcvSpeed); 
}

Int32 TcpFileCli::uploadFile(EvMsgStartUpload* msg) {
    Int32 ret = 0;

    ret = m_ctx->uploadFile(msg);
    return ret;
}

Int32 TcpFileCli::downloadFile(EvMsgStartDownload* msg) {
    Int32 ret = 0;

    ret = m_ctx->downloadFile(msg);
    return ret;
}


TcpSrvTest::TcpSrvTest() {
    m_poll = NULL;
    m_handler = NULL;
}

TcpSrvTest::~TcpSrvTest() {
}

Void TcpSrvTest::setParam(FilePoll* poll, I_Handler* handler) { 
    m_poll = poll;
    m_handler = handler;
}

Int32 TcpSrvTest::addCliOper(Int32 fd) {
    Int32 ret = 0;
    TcpBaseOper* cliOper = NULL;

    I_NEW(TcpBaseOper, cliOper);

    cliOper->setParam(m_handler);
    ret = m_poll->addOper(fd, cliOper, TRUE, TRUE, TRUE);
    if (0 != ret) {
        return -1;
    }

    return 0;
}

