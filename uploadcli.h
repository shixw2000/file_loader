#ifndef __UPLOADCLI_H__
#define __UPLOADCLI_H__
/****
    * This File is used by all of file sender state machine.
****/
#include"istate.h"
#include"filectx.h"
#include"eventmsg.h"


class I_Ctx;
class Engine; 
class FileReader;
class Sender;

class SenderBase : public I_State {
public:
    SenderBase();
    virtual ~SenderBase();

protected:
    virtual Void prepareEx() {}
    virtual Void processEx(Void*) = 0;
    virtual Void dealQuarterTimer() {}
    virtual Void dealReportTimer() {}

private:
    virtual Void prepare(I_Ctx*);
    virtual Void process(Void*); 

protected:
    Sender* m_ctx;
    Engine* m_eng;
    FileReader* m_reader;
};

class SenderInit : public SenderBase {
protected: 
    virtual Void processEx(Void*);

private:
    Void parseUpload(const EvMsgStartUpload* info);
    Void parseDownload(const EvMsgReqDownload* info);
};

class UploadCliConn : public SenderBase {
protected:
    virtual Void prepareEx();
    virtual Void processEx(Void*);
    virtual Void post();

private:
    Void dealUploadAck(EvMsgAckUpload* msg);
    Void dealParamAck(EvMsgAckParam* msg);
    
    EvMsgReqUpload* buildReq(); 
    EvMsgExchParam* buildExchParam(); 
    Void parseParamAck(const EvMsgAckParam* info);
};


class DownloadSrvAccept : public SenderBase {
protected: 
    virtual Void prepareEx();
    virtual Void processEx(Void*);
    virtual Void post();

private:
    Void dealParamAck(EvMsgAckParam* msg);
    
    EvMsgAckDownload* buildAck(); 
    Void parseParamAck(const EvMsgAckParam* info);
};

class TaskSetupSender : public SenderBase {
protected:
    virtual Void prepareEx();
    virtual Void processEx(Void* msg);
};


class TaskSendFinish : public SenderBase {
protected:
    virtual Void prepareEx();
    virtual Void processEx(Void*);
    virtual Void post();

protected:
    Int32 notifyFinish();
    Void dealFinish(EvMsgTaskCompleted* msg); 
};


/* adjust frame size to appropriate value to adapt the speed ratio */
class TaskSlowStartup : public SenderBase { 
protected:
    virtual Void prepareEx();
    virtual Void processEx(Void*);

protected:
    Void dealBlkAck(EvMsgTransDataAck* pRsp);
    Bool correct(const EvMsgTransDataAck* pRsp, TransBaseType* data);
    Void sendBlk();
    Void dealQuarterTimer();
    Void dealReportTimer();
        
private:
    Int32 m_fast_counter;
    Int32 m_slow_counter;
};

/* adjust congestion window size to appropriate value to adapt the speed ratio */
class TaskAvoidCongest : public SenderBase { 
protected:
    virtual Void prepareEx();
    virtual Void processEx(Void*);

protected:
    Void dealQuarterTimer();
    Void dealReportTimer();
    Void dealBlkAck(EvMsgTransDataAck* pRsp);
    Void sendBlk();
    Void parseReportParam(Void* msg);
    Bool chkAutoconf(TransBaseType* data);

private:
    Int32 m_send_cnt;
};


class TaskRetrans : public SenderBase {
protected:
    virtual Void prepareEx();
    virtual Void processEx(Void*);

protected:
    Void dealQuarterTimer();
    Void dealReportTimer();
    Void dealBlkAck(EvMsgTransDataAck* pRsp);
    Void dealRetrans();
    Void retransmission();
    Bool needRetrans();

private:
    Int32 m_retrans_cnt;
    Bool m_allow;
};


/* adjust frame size to appropriate value to adapt the speed ratio */
class TaskSendTest : public SenderBase { 
protected:
    virtual Void prepareEx();
    virtual Void processEx(Void*);

protected:
    Void dealBlkAck(EvMsgTransDataAck* pRsp);
    Void sendBlk();
    Void dealQuarterTimer();
    Void dealReportTimer();
};


#endif

