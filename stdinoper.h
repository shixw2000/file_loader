#ifndef __STDINOPER_H__
#define __STDINOPER_H__
#include"globaltype.h"
#include"ihandler.h"


class FilePoll;
class StdinOper : public I_FdOper {
public:
    StdinOper();
    ~StdinOper();

    Void setParam(Int32 dst, FilePoll* poll); 

    Int32 readFd(PollItem* item);
    Int32 writeFd(PollItem*) {return -1;}
    Int32 dealFd(PollItem* item); 

private:
    FilePoll* m_poll;
    Int32 m_dst; 
};


#endif

