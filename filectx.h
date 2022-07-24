#ifndef __FILECTX_H__
#define __FILECTX_H__
#include"globaltype.h"


/****
    * this is a configuration type used by Cmdctx,
    * most of it's members are derived from the user 
    * specifications. 
****/
typedef struct { 
    Int32 m_mss;
    Int32 m_max_cwnd; 
    
    Int32 m_conf_send_speed;
    Int32 m_conf_recv_speed; 
    Int32 m_upload_thr_max;
    Int32 m_download_thr_max;

    Int32 m_upload_thr_cnt;
    Int32 m_download_thr_cnt;
    Int32 m_max_send_speed;
    Int32 m_max_recv_speed;
} TaskConfType;

enum {
    ENUM_FILE_TYPE_REG,
    ENUM_FILE_TYPE_DIR,

    ENUM_FILE_TYPE_OTHER
};

typedef struct {
    Int64 m_file_size;
    Int32 m_file_type;
} FileInfoType;


/****
    * This is the most important data type,
    * It is used by the state machine of upload 
    * and download.
****/
typedef struct {
    Int64 m_file_size; 
    Uint32 m_file_crc; 
    Int32 m_blk_cnt; 
    
    Int32 m_file_unit;
    Int32 m_next_offset;
    Int32 m_finish_offset;

    Uint32 m_random;
    Int32 m_blk_next;
    Int32 m_blk_next_ack;
    Int32 m_blk_beg;
    Int32 m_blk_end;
    
    Int32 m_max_speed;
    Int32 m_last_error;
    Int32 m_frame_size;
    Int32 m_completed;

    Int32 m_upload_download;
    Int32 m_send_recv;
    
    Int32 m_file_fd[ENUM_FILE_BUTT];

    Char m_file_path[MAX_FILEPATH_SIZE];
    Char m_file_name[MAX_FILENAME_SIZE];
    Char m_file_id[MAX_FILEID_SIZE];
    Char m_task_id[MAX_TASKID_SIZE];
} TransBaseType;


#pragma pack(push, 1)

/****
    * This is a file header of the map file,
    * it records the file basic information with the data file.
****/
struct FileMapHeader {
    Int64 m_file_size;
    Uint32 m_magic_flag;
    Uint32 m_file_crc; 
    Int32 m_file_unit;
    Int32 m_blk_cnt;
    Int32 m_blk_next;
    Int32 m_completed;
    Char m_file_name[MAX_FILENAME_SIZE];
    Char m_file_id[MAX_FILEID_SIZE];
};

#pragma pack(pop)

void resetTaskConf(TaskConfType* data);
Void resetTransData(TransBaseType* data);
Void closeTransData(TransBaseType* data);
void buildMapHeader(const TransBaseType* data, FileMapHeader* info);
Void buildFileID(const Char task_id[], TransBaseType* data);


#endif

