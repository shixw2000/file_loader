#ifndef __FILEWRITER_H__
#define __FILEWRITER_H__
/****
    * This file is used for receiving data and store files.
    * All file datas transferred from network are dealed here.
    * The class FileWriter may cache fragmentary file blocks
    * to a continuas 4MB size,  then write the cache total to file once. 
    *
    * only data msg of EvMsgTransData would be handled in the class.
****/
#include"globaltype.h"
#include"listnode.h"
#include"filectx.h"
#include"eventmsg.h"


class Engine;

class FileWriter {
public:
    explicit FileWriter(Engine* eng);
    ~FileWriter(); 

    /* must be called before any operations */
    Int32 start();

    /* must be called after a end usage before free */
    Void stop();

    Void prepareRecv(TransBaseType* data);

    Int32 notifyRecv(EvMsgTransData* pReq, TransBaseType* data); 
    Int32 dealBlkFinish(TransBaseType* data); 

    Int32 prepareFileMap(TransBaseType* data);
    Int32 creatFileMap(TransBaseType* data);
    Int32 writeFileMap(TransBaseType* data);
    
protected: 
    Int32 setFinishFlag(TransBaseType* data); 
    Int32 writeFile(TransBaseType* data);
    Void updateNextAck(TransBaseType* data);
    Void moveBlk(TransBaseType* data);
    Bool canWrite(TransBaseType* data);
    Bool endOfFile(TransBaseType* data);
    Int32 writeData(TransBaseType* data);
    Void pushBlk(EvMsgTransData* pReq, TransBaseType* data);
    Bool popBlk(TransBaseType* data); 

private:
    Engine* m_eng; // This is a referenced parameter, for msg send, and so on.
    EvMsgTransData* m_curr_blk; // the next block to write, recycled by class itself.
    order_list_head m_list; // data block for receiving, ordered by least begin offset
    
    Int32 m_blk_offset; // this following three parameters are used by caching operation
    Int32 m_blk_pos; 
    Char m_buf[DEF_MAX_BLK_CNT * DEF_FILE_UNIT_SIZE];
};

#endif

