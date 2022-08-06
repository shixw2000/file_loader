#ifndef __FILEPOLL_H__
#define __FILEPOLL_H__
#include"globaltype.h"
#include"listnode.h"
#include"ihandler.h"


struct pollfd;

enum EnumPollStatus {
    ENUM_POLL_STATUS_NULL = 0,
    ENUM_POLL_IDLE = 1,
    ENUM_POLL_WAIT,
    ENUM_POLL_RUNNING,
    
};

class FilePoll {
public:
    FilePoll();
    ~FilePoll();
    
    Int32 init();
    Void finish(); 
    
    Int32 run(); 

    Int32 delOper(Int32 fd);
    Int32 addOper(Int32 fd, I_FdOper* oper, Bool bDel, 
        Bool bRd, Bool bWr); 
    Int32 addSendQue(Int32 fd, Void* msg);

    Void startRead(Int32 fd);
    Void stopWrite(Int32 fd);

protected:   
    Void dealItem(PollItem* item);
    Int32 chkEvent(); 
    Void initItem(PollItem* pos);
    Void finishItem(PollItem* pos);

private:
    const Int32 m_capacity;
    
    struct pollfd* m_fds;
    PollItem* m_items; 
    list_head m_run_list;
    list_head m_wait_list;
    list_head m_idle_list; 
    Bool m_fast_round;
};

#endif

