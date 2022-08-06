#ifndef __TCPCMDCTX_H__
#define __TCPCMDCTX_H__
#include"globaltype.h"
#include"ihandler.h"
#include"ievent.h"
#include"eventoper.h"


class TcpSignalOper;
class FilePoll;
class TickTimer;
class TcpBaseOper;
class CmdCtx;

class TcpBaseOper : public I_FdOper {
public:
    TcpBaseOper();
    ~TcpBaseOper();

    Void setParam(I_Handler* handler);

    Void finish(); 

    Int32 readFd(PollItem* item);
    Int32 writeFd(PollItem* item);
    Int32 dealFd(PollItem* item); 

protected: 
    Bool chkHeader(const EvMsgHeader* hd);
    Int32 parse(const Char* buf, int len, PollItem* item);

    Int32 fillMsg(const Char* buf, int len, PollItem* item);

private:
    I_Handler* m_handler;
    Void* m_snd_msg;
    Void* m_rcv_msg;
    
    Int32 m_header_len; 
    Char m_header[DEF_MSG_HEADER_SIZE];
    
    static char m_tcp_buf[DEF_TCP_RECV_BUF_SIZE];
};


class TcpCmdCtx : public I_Comm, public I_Obj, 
    public I_Handler, public I_FdOper { 
public:
    TcpCmdCtx();
    ~TcpCmdCtx();

    Void setParam(Int32 fd, FilePoll* poll, CmdCtx* ctx);
        
    Int32 init();
    Void finish();

    virtual Int32 sendPkg(Void* msg);
    virtual Int32 transPkg(Void* msg);
    virtual Void signal();
    virtual Void reportResult(const ReportFileInfo* info);

    virtual Void event(Uint32 val);
    virtual Void process(Void* msg);

    Int32 readFd(PollItem* item);
    Int32 writeFd(PollItem* item);
    Int32 dealFd(PollItem*);

    Bool eof() const { return m_end; }
    Void setEof() { m_end = TRUE; }

private:
    FilePoll* m_poll;
    CmdCtx* m_ctx;
    
    TcpSignalOper* m_ev_oper; 
    TcpBaseOper* m_tcp_oper; 
    Int32 m_tcp_fd;
    Int32 m_ev_fd;
    Bool m_end;
};


class TcpSignalOper : public SignalOper {
public:
    TcpSignalOper();
    ~TcpSignalOper();

    Void setParam(Int32 fd, TcpCmdCtx* cb);
    
    Void finish();

private:
    TcpCmdCtx* m_cb;
};

#endif

