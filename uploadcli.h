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

class SenderInit : public I_State {
public:
    SenderInit();
    ~SenderInit();
    
    virtual Void prepare(I_Ctx*);
    
    virtual Void process(Void*);

private:
    Void parseUpload(const EvMsgStartUpload* info);
    Void parseDownload(const EvMsgReqDownload* info);
    
private:
    I_Ctx* m_ctx;
    Engine* m_eng;
    FileReader* m_reader;
};

class UploadCliConn : public I_State {
public:
    UploadCliConn();
    ~UploadCliConn();
    
    virtual Void prepare(I_Ctx*);
    virtual Void process(Void*);
    virtual Void post();

private:
    Void dealUploadAck(EvMsgAckUpload* msg);
    Void dealParamAck(EvMsgAckParam* msg);
    
    EvMsgReqUpload* buildReq(); 
    EvMsgExchParam* buildExchParam(); 
    Void parseParamAck(const EvMsgAckParam* info);
    
private:
    I_Ctx* m_ctx;
    Engine* m_eng;
    FileReader* m_reader;
    Void* m_timerID;
};


class DownloadSrvAccept : public I_State {
public:
    DownloadSrvAccept();
    ~DownloadSrvAccept();
    
    virtual Void prepare(I_Ctx*);
    virtual Void process(Void*);
    virtual Void post();

private:
    Void dealParamAck(EvMsgAckParam* msg);
    
    EvMsgAckDownload* buildAck(); 
    Void parseParamAck(const EvMsgAckParam* info);
    
private:
    I_Ctx* m_ctx;
    Engine* m_eng;
    FileReader* m_reader;
    Void* m_timerID;
};


class TaskSetupSender : public I_State {
public:
    TaskSetupSender();
    ~TaskSetupSender();
    
    virtual Void prepare(I_Ctx*);
    virtual Void process(Void*);
    virtual Void post();

protected:
    Void dealBlkAck(EvMsgTransDataAck* pRsp);
    Void prepareSend();
    Void sendBlk(TransBaseType* data);

private:
    I_Ctx* m_ctx;
    Engine* m_eng;
    FileReader* m_reader;
    Void* m_timerID;
};


class TaskSendFinish : public I_State {
public:
    TaskSendFinish();
    ~TaskSendFinish();
    
    virtual Void prepare(I_Ctx*);
    virtual Void process(Void*);
    virtual Void post();

protected:
    Int32 notifyFinish();
    Void dealFinish(EvMsgTransAckFinish* msg);

private:
    I_Ctx* m_ctx;
    Engine* m_eng;
    FileReader* m_reader;
    Void* m_timerID;
};

#endif

