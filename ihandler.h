#ifndef __IHANDLER_H__
#define __IHANDLER_H__
#include"globaltype.h"
#include"listnode.h"


class I_FdOper;

struct PollItem {
    list_head m_snd_msgs;
    list_head m_rcv_msgs;
    list_node m_node;
    I_FdOper*  m_oper;  
    
    Int32 m_fd;
    Int32 m_status;
    Bool m_can_write;
    Bool m_test_read;
    Bool m_more_read;
    Bool m_more_write;
    Bool m_error; 
    Bool m_del_oper;
};

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

class I_Comm {
public:
    virtual ~I_Comm() {}
    virtual Int32 sendPkg(Void* msg) = 0;
    virtual Int32 transPkg(Void* msg) = 0;
    virtual Void signal() = 0;
    virtual Void reportResult(const ReportFileInfo* info) = 0; 
};


class I_Handler {
public:
    virtual ~I_Handler() {}

    virtual Void process(Void* msg) = 0;
};

class I_Obj {
public:
    virtual ~I_Obj() {}
    virtual Void event(Uint32 val) = 0;
};


class I_FdOper {
public:
    virtual ~I_FdOper() {}

    virtual Int32 init() { return 0; }
    virtual Void finish() {}
    
    virtual Int32 readFd(PollItem* item) = 0;
    virtual Int32 writeFd(PollItem* item) = 0;
    virtual Int32 dealFd(PollItem* item) = 0;
};


#endif

