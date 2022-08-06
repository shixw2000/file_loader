#ifndef __TCPOPER_H__
#define __TCPOPER_H__
#include"globaltype.h"
#include"ihandler.h"
#include"ievent.h"
#include"cthread.h"
#include"tcpcmdctx.h"
#include"eventmsg.h"


class FilePoll;
class TickTimer;
class TimerOper;
class CmdCtx;

class TcpSrvOper : public I_FdOper {
public: 
    Int32 readFd(PollItem* item);
    Int32 writeFd(PollItem*) {return -1;}
    Int32 dealFd(PollItem*) {return -1;} 

protected:
    virtual Int32 addCliOper(Int32 fd) = 0;
};


class TcpFileSrv : public TcpSrvOper, public I_Element {
public:
    TcpFileSrv();
    virtual ~TcpFileSrv();

    Int32 init();
    Void finish(); 

    Int32 start();
    Void stop();

    Void setParam(const Char ip[], int port);

    Void setConfSpeed(Int32 sndSpeed, Int32 rcvSpeed);

protected:
    virtual Int32 addCliOper(Int32 fd);
    virtual Int32 run();

private:    
    CThread* m_thr;
    FilePoll* m_poll;
    TickTimer* m_timer;
    TimerOper* m_timer_oper;
    Int32 m_timer_fd;
    Int32 m_listen_fd;
    Int32 m_port;
    Int32 m_conf_send_speed;
    Int32 m_conf_recv_speed;
    Char m_ip[DEF_IP_SIZE];
};

class TcpFileCli : public TcpCmdCtx, public I_Element {
public:
    TcpFileCli();
    virtual ~TcpFileCli();

    Void setParam(const Char ip[], int port);

    Int32 init();
    Void finish(); 

    Int32 start();
    Void stop(); 

    Void setConfSpeed(Int32 sndSpeed, Int32 rcvSpeed);

    Int32 uploadFile(EvMsgStartUpload* msg);
    Int32 downloadFile(EvMsgStartDownload* msg);

protected:
    virtual Int32 run();

private:    
    CThread* m_thr;
    FilePoll* m_poll;
    TickTimer* m_timer;
    CmdCtx* m_ctx;
    TimerOper* m_timer_oper;
    Int32 m_timer_fd;
    Int32 m_cli_fd;
    int m_port;
    Char m_ip[DEF_IP_SIZE];
};


class TcpSrvTest : public TcpSrvOper {
public:
    TcpSrvTest();
    virtual ~TcpSrvTest();

    Void setParam(FilePoll* poll, I_Handler* handler);

protected:
    virtual Int32 addCliOper(Int32 fd);

private:   
    FilePoll* m_poll;
    I_Handler* m_handler;
};

#endif

