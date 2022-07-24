#ifndef __GLOBALTYPE_H__
#define __GLOBALTYPE_H__
/****
    * This file is a global header file
****/
#include<stdio.h>


#define MAX_TASKID_SIZE 32
#define MAX_FILEID_SIZE 32
#define MAX_FILENAME_SIZE 128
#define MAX_FILEPATH_SIZE 256
#define MAX_MAC_CODE_SIZE 32

#define DEF_DATA_FLIGHT_SIZE 10
#define DEF_MAGIC_NUM 0x1A2D3C4E


#define DEF_FILE_UNIT_SIZE 0x1000
#define DEF_FILE_UNIT_MASK 0xFFF
#define DEF_FILE_UNIT_SHIFT_BITS 12

#define DEF_TICK_TIME_MS 50
#define DEF_TICK_PER_SEC 20
#define DEF_ONE_FOURTH_SEC_TICK_CNT 5 // 250ms

#define DEF_TIMER_RATIO_SIZE 32
#define DEF_TIMER_RATIO_MASK 0x1F
#define DEF_TIMER_RATIO_SHIFT_NUM 5

#define DEF_MAX_FRAME_SIZE 256      // 1MB
#define DEF_MAX_CWND_SIZE 12
#define DEF_MIN_CWND_SIZE 6
#define DEF_MAX_SPEED_SIZE 0x8000 // ~ 1Gb/s
#define DEF_TEST_SPEED_SIZE 0x400 // 4MB/s
#define DEF_MAX_THR_SIZE 4

#define DEF_MAX_BLK_CNT 0x400

typedef unsigned char Byte;
typedef char Char;
typedef short Int16;
typedef unsigned short Uint16;
typedef int Int32;
typedef unsigned int Uint32;
typedef long long Int64;
typedef unsigned long long Uint64;
typedef bool Bool;
typedef void Void;

typedef Void (*PFunc)(Void* p1, Void* p2);

enum BOOL_VAL {
    FALSE = 0,
    TRUE = 1
};


#ifndef NULL
#define NULL 0
#endif

#define I_FREE(x) do { if (NULL != (x)) {delete x; x=NULL;} } while (0)
#define I_NEW(type, x)  ((x) = new type)
#define I_NEW_1(type, x, val)  ((x) = new type(val))
#define I_NEW_2(type, x, v1, v2)  ((x) = new type(v1, v2))
#define ARR_FREE(x) do { if (NULL != (x)) {delete[] x; x=NULL;} } while (0)
#define ARR_NEW(type,size, x) ((x) = new type[size])


#define offset_of(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member)* __mptr = (ptr);	\
	(type *)((char*)__mptr - offset_of(type, member)); })

#define LOG_INFO(format,args...) \
    fprintf(stdout, format"|\n", ##args)


enum EnumErrCode {
    ENUM_ERR_OK = 0,
        
    ENUM_ERR_CMD_UNKNOWN,
    ENUM_ERR_TIMEOUT,
    ENUM_ERR_PARAM_INVALID,

    ENUM_ERR_WRITE_BLK,
    ENUM_ERR_READ_BLK,

    ENUM_ERR_PKG_INVALID,
};

enum EnumFileType {
    ENUM_FILE_DATA = 0,
    ENUM_FILE_MAP,

    ENUM_FILE_BUTT
};

enum {
    ENUM_TASK_TYPE_UPLOAD = 100,
    ENUM_TASK_TYPE_DOWNLOAD = 101,

    ENUM_DATA_TYPE_SEND = 200,
    ENUM_DATA_TYPE_RECV = 201,
};

enum EnumCmdState {
    ENUM_TASK_INIT = 0,
        
    ENUM_TASK_CONN_UPLOAD,
    ENUM_TASK_CONN_DOWNLOAD,
    
    ENUM_TASK_ACCEPT_UPLOAD,
    ENUM_TASK_ACCEPT_DOWNLOAD,

    ENUM_TASK_SETUP_SENDER,
    ENUM_TASK_SETUP_RECEIVER,

    ENUM_TASK_SEND_FINISH,
    ENUM_TASK_RECV_FINISH,

    ENUM_TASK_AVOID_CONG_SENDER,
    ENUM_TASK_AVOID_CONG_RECEIVER,

    ENUM_TASK_FAIL_END,
    ENUM_TASK_SUCCESS_END,
    
    ENUM_TASK_BUTT
};

static const Uint32 DEF_CMD_TIMEOUT_SEC = DEF_TICK_PER_SEC * 2;
static const Uint32 DEF_BLOCK_TIMEOUT_SEC = DEF_TICK_PER_SEC * 15;

#endif

