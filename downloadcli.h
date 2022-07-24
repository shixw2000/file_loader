#ifndef __DOWNLOADCLI_H__
#define __DOWNLOADCLI_H__
#include"globaltype.h"
#include"istate.h" 
#include"eventmsg.h"


class FileWriter;
class I_Ctx;
class Engine; 

class ReceiverInit : public I_State {
public:
    ReceiverInit();
    ~ReceiverInit();
    
    virtual Void prepare(I_Ctx*);
    virtual Void process(Void*);

private:
    Void parseDownload(const EvMsgStartDownload* info);
    Void parseUpload(const EvMsgReqUpload* info);
    
private:
    I_Ctx* m_ctx;
    Engine* m_eng;
    FileWriter* m_writer;
};

class DownloadCliConn : public I_State {
public:
    DownloadCliConn();
    ~DownloadCliConn();
    
    virtual Void prepare(I_Ctx*);
    virtual Void process(Void*);
    virtual Void post();

private:
    EvMsgReqDownload* buildReq();
    Void parseAck(const EvMsgAckDownload* info);
    EvMsgAckParam* buildAckParam();
    Void dealDownloadAck(EvMsgAckDownload* msg);

private:
    I_Ctx* m_ctx;
    Engine* m_eng;
    FileWriter* m_writer;
    Void* m_timerID;
};


class UploadSrvAccept : public I_State {
public:
    UploadSrvAccept();
    ~UploadSrvAccept();
    
    virtual Void prepare(I_Ctx*);
    virtual Void process(Void* msg);
    virtual Void post();

private:
    EvMsgAckUpload* buildAck();
    Void dealExchParam(EvMsgExchParam* pReq);
    Void parseParam(const EvMsgExchParam* info);
    EvMsgAckParam* buildAckParam();

private:
    I_Ctx* m_ctx;
    Engine* m_eng;
    FileWriter* m_writer;
    Void* m_timerID;
};


class TaskSetupReceiver : public I_State {
public:
    TaskSetupReceiver();
    ~TaskSetupReceiver();
    
    virtual Void prepare(I_Ctx*);
    virtual Void process(Void* msg);
    virtual Void post();

private:
    Void recvBlkData(EvMsgTransData* msg);
    Void dealFinish(EvMsgTransBlkFinish* msg);

private:
    I_Ctx* m_ctx;
    Engine* m_eng;
    FileWriter* m_writer;
    Void* m_timerID;
};

class TaskRecvFinish : public I_State {
public:
    TaskRecvFinish();
    ~TaskRecvFinish();
    
    virtual Void prepare(I_Ctx*);
    virtual Void process(Void*);
    virtual Void post();

private:
    Void notifyFinish();

private:
    I_Ctx* m_ctx;
    Engine* m_eng;
    FileWriter* m_writer;
    Void* m_timerID;
};


#endif

