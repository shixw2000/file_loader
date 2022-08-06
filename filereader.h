#ifndef __FILEREADER_H__
#define __FILEREADER_H__
/****
    * This file is used for reading and sending file data.
    * It reads 4MB block once a time, then cut fragmentary
    * frame block to send, and repeated this process recursively.
    * 
****/
#include"globaltype.h"
#include"listnode.h"
#include"filectx.h"
#include"eventmsg.h"


class Engine;

class FileReader {
public:
    explicit FileReader(Engine* eng);
    ~FileReader();

    /* This function must be called before all of others */
    Int32 start();

    /* This function must be called after the end of usage before free */
    Void stop(); 

    Int32 waitSize() const;
    EvMsgBlkRange* getLastWait(); 
    Int32 retransFrame(const EvMsgBlkRange* pBlk, TransBaseType* data);

    Void prepareSend(TransBaseType* data);
    Void postSend(TransBaseType* data);
    
    Void purgeWaitList(const EvMsgTransDataAck* pRsp, TransBaseType* data); 

    Int32 parseRawFile(TransBaseType* data);
    Int32 parseMapFile(TransBaseType* data);
    Int32 parseFile(const Char path[], TransBaseType* data);

    /* push a frame of range to send list */
    Bool pushBlk(TransBaseType* data);

    /* send block datas according send list */
    Int32 sendBlk(TransBaseType* data);

private:
    Int32 grow(TransBaseType* data); 
    Bool popBlk(TransBaseType* data);
    Bool fillBlk(TransBaseType* data);

private:
    Engine* m_eng;
    EvMsgTransData* m_curr_blk;
    order_list_head m_list_read; 
    order_list_head m_list_wait; 
    
    Int32 m_blk_offset;
    Int32 m_blk_size;
    Int32 m_blk_pos;
    Char m_buf[DEF_MAX_BLK_CNT * DEF_FILE_UNIT_SIZE];
};

#endif

