#include<string.h>
#include"filectx.h" 
#include"filetool.h"


void resetTaskConf(TaskConfType* data) {
    memset(data, 0, sizeof(TaskConfType));

    data->m_mss = DEF_MAX_FRAME_SIZE;
    data->m_max_cwnd = DEF_MAX_CWND_SIZE; 
    
    data->m_conf_send_speed = DEF_TEST_SPEED_SIZE;
    data->m_conf_recv_speed = DEF_TEST_SPEED_SIZE; 

    data->m_upload_thr_max = DEF_MAX_THR_SIZE;
    data->m_download_thr_max = DEF_MAX_THR_SIZE;

    data->m_upload_thr_cnt = 0;
    data->m_download_thr_cnt = 0;

    data->m_max_send_speed = data->m_conf_send_speed;
    data->m_max_recv_speed = data->m_conf_recv_speed;
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


