#ifndef __DOWNLOADCLI_H__
#define __DOWNLOADCLI_H__
#include"globaltype.h"
#include"istate.h" 
#include"eventmsg.h"


class FileWriter;
class I_Ctx;
class Engine; 
class Receiver;

class ReceivBase : public I_State {
public:
    ReceivBase();
    virtual ~ReceivBase();

protected:
    virtual Void prepareEx() {}
    virtual Void processEx(Void*) = 0;
    virtual Void dealQuarterTimer() {}
    virtual Void dealReportTimer() {}

private:
    virtual Void prepare(I_Ctx*);
    virtual Void process(Void*); 

protected:
    Receiver* m_ctx;
    Engine* m_eng;
    FileWriter* m_writer;
};

class ReceiverInit : public ReceivBase {
protected: 
    virtual Void processEx(Void*);

private:
    Void parseDownload(const EvMsgStartDownload* info);
    Void parseUpload(const EvMsgReqUpload* info);
};

class DownloadCliConn : public ReceivBase { 
protected: 
    virtual Void prepareEx();
    virtual Void processEx(Void*);
    virtual Void post();

private:
    EvMsgReqDownload* buildReq();
    Void parseAck(const EvMsgAckDownload* info);
    EvMsgAckParam* buildAckParam();
    Void dealDownloadAck(EvMsgAckDownload* msg);

};


class UploadSrvAccept : public ReceivBase {
protected: 
    virtual Void prepareEx();
    virtual Void processEx(Void*);
    virtual Void post();

private:
    EvMsgAckUpload* buildAck();
    Void dealExchParam(EvMsgExchParam* pReq);
    Void parseParam(const EvMsgExchParam* info);
    EvMsgAckParam* buildAckParam();
};


class TaskSetupReceiver : public ReceivBase {
protected: 
    virtual Void prepareEx();
    virtual Void processEx(Void*);

private:
    Void recvBlkData(EvMsgTransData* msg);
    Void dealFinish(EvMsgTransBlkFinish* msg);
    Void dealQuarterTimer();
    Void dealReportTimer();
};

class TaskRecvFinish : public ReceivBase {
protected: 
    virtual Void prepareEx();
    virtual Void processEx(Void*);
    virtual Void post();

private:
    Void notifyFinish();
    Void dealFinish(EvMsgTaskCompleted* pRsp);
    Void dealQuarterTimer();
};


#endif

