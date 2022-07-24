#ifndef __EVENTMSG_H__
#define __EVENTMSG_H__
/*****
    * This file is used by all of Messages.
    * It not contains of the msg header, which
    * is defined in ievent.h.
    * each message has a Cmd type as counterpart.
*****/
#include"ievent.h"


enum EnumCmdType {
    CMD_START_FILE_UPLOAD,
    CMD_START_FILE_DOWNLOAD,
    CMD_REQ_FILE_UPLOAD,
    CMD_REQ_FILE_DOWNLOAD,
    CMD_ACK_FILE_UPLOAD,
    CMD_ACK_FILE_DOWNLOAD,
    CMD_EXCH_PARAM,
    CMD_ACK_PARAM,

    CMD_TRANS_DATA_BLK,
    CMD_TRANS_DATA_BLK_ACK,

    CMD_DATA_BLK_RANGE,
    CMD_SEND_BLK_FINISH,
    CMD_RECV_BLK_FINISH, 

    CMD_ALARM_TRANS_BLK,

    CMD_START_BLK_TRANS,
    CMD_STOP_BLK_TRANS,

    CMD_TASK_DEAL_FAIL,

    CMD_ALARM_ONE_TICK,
    CMD_ALARM_TICK_TIMEOUT,
    CMD_ALARM_TICK_ONE_FOURTH_SEC,

    CMD_FILE_TRANS_RESULT_REPORT,
    
    CMD_TYPE_NULL
};


#pragma pack(push, 1)


typedef struct {
    Int64 m_file_size;
    Uint32 m_file_crc;
    
    Char m_file_name[MAX_FILENAME_SIZE]; 
} EvMsgStartUpload;


typedef struct {
    Char m_file_id[MAX_FILEID_SIZE];
} EvMsgStartDownload;


typedef struct {    
    Int64 m_file_size;
    Uint32 m_file_crc;
    Uint32 m_random;
    
    Char m_file_name[MAX_FILENAME_SIZE];
    Char m_mac_code[MAX_MAC_CODE_SIZE]; 
} EvMsgReqUpload;


typedef struct {
    Uint32 m_random;
    
    Char m_mac_code[MAX_MAC_CODE_SIZE];
    Char m_file_id[MAX_FILEID_SIZE];
} EvMsgReqDownload;


typedef struct {    
    Int64 m_file_size;
    Uint32 m_file_crc;

    Int32 m_max_speed;
    Uint32 m_random;
  
    Char m_file_name[MAX_FILENAME_SIZE];
} EvMsgAckDownload;

typedef struct { 
    Uint32 m_random;
} EvMsgAckUpload;

typedef struct {
    Uint32 m_random;
    Int32 m_max_speed;
} EvMsgExchParam;

typedef struct {
    Uint32 m_random;
    Int32 m_max_speed;
    Int32 m_blk_beg;
    Int32 m_blk_end;
} EvMsgAckParam;


typedef struct {    
    Int32 m_blk_beg;
    Int32 m_blk_end;
    Int32 m_buf_size;
    Int32 m_time;
    Char m_buf[0];
} EvMsgTransData;


typedef struct {    
    Int32 m_blk_next;
    Int32 m_time;
} EvMsgTransDataAck;


typedef struct {    
    Int32 m_blk_beg;
    Int32 m_blk_end;
    Int32 m_time;
} EvMsgBlkRange;

typedef struct {
    Int32 m_blk_beg;
    Int32 m_blk_end;
} EvMsgTransBlkFinish;

typedef struct {
    Int32 m_blk_beg;
    Int32 m_blk_end;

    Char m_file_id[MAX_FILEID_SIZE];
} EvMsgTransAckFinish;

/****
    * This is the data type of report.
    * attention: 1 byte aligned for a event msg
****/
typedef struct {
    Int64 m_file_size; 
    Int32 m_result;
    Int32 m_upload_download;
    Int32 m_send_recv;
    Char m_file_path[MAX_FILEPATH_SIZE];
    Char m_file_name[MAX_FILENAME_SIZE];
    Char m_file_id[MAX_FILEID_SIZE];
    Char m_task_id[MAX_TASKID_SIZE];
} ReportFileInfo;

typedef ReportFileInfo EvMsgResultReport;

#pragma pack(pop)

#endif

