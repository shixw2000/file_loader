#ifndef __IEVENT_H__
#define __IEVENT_H__
#include"globaltype.h"


#pragma pack(push, 1)

/* The message header of each evmsg */
typedef struct {
    Int32 m_size;
    Int32 m_ev;
    Int32 m_code;
    Char m_task_id[MAX_TASKID_SIZE];
} EvMsgHeader; 

/* the default dummy msg */
typedef struct {} I_Event;

#pragma pack(pop)

static const Int32 DEF_MSG_HEADER_SIZE = sizeof(EvMsgHeader);

#endif

