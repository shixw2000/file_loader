#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<time.h>
#include"filectx.h" 
#include"filetool.h"


void resetTaskConf(TaskConfType* data) {
    memset(data, 0, sizeof(TaskConfType));

    data->m_mss = DEF_MAX_FRAME_SIZE;
    data->m_max_cwnd = DEF_MAX_CWND_SIZE; 
    
    data->m_conf_send_speed = 0;
    data->m_conf_recv_speed = 0; 

    data->m_send_thr_max = DEF_MAX_THR_SIZE;
    data->m_recv_thr_max = DEF_MAX_THR_SIZE;

    data->m_send_thr_cnt = 0;
    data->m_recv_thr_cnt = 0;
}

Void resetTransData(TransBaseType* data) {
    memset(data, 0, sizeof(TransBaseType)); 

    data->m_file_unit = DEF_FILE_UNIT_SIZE;

    data->m_next_offset = (Int32)offset_of(FileMapHeader, m_blk_next);
    data->m_finish_offset = (Int32)offset_of(FileMapHeader, m_completed);
        
    data->m_file_fd[ENUM_FILE_DATA]= -1;
    data->m_file_fd[ENUM_FILE_MAP] = -1;
}

Void closeTransData(TransBaseType* data) {
    FileTool::closeFd(&data->m_file_fd[ENUM_FILE_DATA]);
    FileTool::closeFd(&data->m_file_fd[ENUM_FILE_MAP]);
}

void buildMapHeader(const TransBaseType* data, FileMapHeader* info) { 
    info->m_file_size = data->m_file_size;
    info->m_file_crc = data->m_file_crc;
    info->m_file_unit = data->m_file_unit;
    info->m_blk_cnt = data->m_blk_cnt;
    info->m_blk_next = data->m_blk_next;
    info->m_completed = data->m_completed; 
    info->m_magic_flag = DEF_MAGIC_NUM;
    
    strncpy(info->m_file_name, data->m_file_name, MAX_FILENAME_SIZE);
    strncpy(info->m_file_id, data->m_file_id, MAX_FILEID_SIZE);
}

Void buildFileID(const Char task_id[], TransBaseType* data) {    
    strncpy(data->m_file_id, task_id, MAX_FILEID_SIZE);
}

Uint32 randTick() {
    Uint32 n = rand();

    return n;
}

Uint32 sys_random() {
    Uint32 n = 0;

    n = getpid();
    return n;
}

Uint64 sys_clock() {
    Uint64 n = 0;
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);
    n = tp.tv_sec * 1000000 + tp.tv_nsec/1000;
    return n;
}

const Char* clockMs() {
    static Char g_time_stamp[32] = {0};
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC_COARSE, &tp);

    snprintf(g_time_stamp, sizeof(g_time_stamp), "%ld.%ld", 
        tp.tv_sec, tp.tv_nsec/1000000);
    
    return g_time_stamp;
}

