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

    Void prepareSend(TransBaseType* data);
    Int32 notifySend(TransBaseType* data);
    Void purgeWaitList(Int32 ackBlk, TransBaseType* data); 

    Int32 parseRawFile(TransBaseType* data);
    Int32 parseMapFile(TransBaseType* data);
    Int32 parseFile(const Char path[], TransBaseType* data);

private:
    Void pushBlk(TransBaseType* data);
    Int32 grow(TransBaseType* data); 
    Bool popBlk(TransBaseType* data);
    Bool fillBlk(TransBaseType* data);
    Int32 sendBlk(TransBaseType* data);

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

