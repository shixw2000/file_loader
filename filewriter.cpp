#include"filewriter.h"
#include"msgcenter.h"
#include"filetool.h"
#include"engine.h"


static Int32 cmpRcvBlk(list_node* n1, list_node* n2) {
    EvMsgTransData* p1 = NULL;
    EvMsgTransData* p2 = NULL;

    p1 = MsgCenter::entry<EvMsgTransData>(n1);
    p2 = MsgCenter::entry<EvMsgTransData>(n2);

    if (p1->m_blk_beg != p2->m_blk_beg) {
        return p1->m_blk_beg - p2->m_blk_beg;
    } else if (p1->m_blk_end != p2->m_blk_end) {
        return p1->m_blk_end - p2->m_blk_end;
    } else {
        return p1->m_time - p2->m_time;
    }
}

FileWriter::FileWriter(Engine* eng) {
    m_eng = eng;
    INIT_ORDER_LIST_HEAD(&m_list, &cmpRcvBlk);

    m_curr_blk = NULL;
    m_blk_offset = 0;
    m_blk_pos = 0;
}

FileWriter::~FileWriter() {
}

Int32 FileWriter::start() {
    Int32 ret = 0;
        
    return ret;
}

Void FileWriter::stop() {
    list_node* pos = NULL;
    list_node* n = NULL;
    EvMsgTransData* pBlk = NULL;

    /* clear unused data */
    list_for_each_safe(pos, n, &m_list) {
        pBlk = MsgCenter::entry<EvMsgTransData>(pos);

        order_list_del(pos, &m_list);
        MsgCenter::freeMsg(pBlk);
    }

    MsgCenter::freeMsg(m_curr_blk);
    m_curr_blk = NULL;
}

Void FileWriter::prepareRecv(TransBaseType* data) {
    m_blk_offset = data->m_blk_next;
    m_blk_pos = 0; 
}

Int32 FileWriter::writeFile(TransBaseType* data) {
    Int64 offset = 0;
    Int32 size = 0;
    Int32 next = 0;
    Int32 ret = 0;

    /* no data */
    if (0 == m_blk_pos) {
        return 0;
    }
    
    next = m_blk_offset + m_blk_pos;
    
    size = FileTool::calcBlkSize(m_blk_offset, next, data->m_file_size); 
    
    offset = m_blk_offset;
    offset <<= DEF_FILE_UNIT_SHIFT_BITS;
    
    /* write data */
    ret = FileTool::pwriteFile(data->m_file_fd[ENUM_FILE_DATA],
        m_buf, size, offset);
    if (0 != ret) {
        return -1;
    } 

    m_blk_offset = next;
    m_blk_pos = 0; 
    
    /* write next flag */
    ret = FileTool::pwriteFile(data->m_file_fd[ENUM_FILE_MAP],
        &next, (Int32)sizeof(Int32), 
        data->m_next_offset);
    if (0 != ret) {
        return -1;
    } 

    /* update next blk written to disk */
    data->m_blk_next = next;
    
    return ret;
}

Int32 FileWriter::setFinishFlag(TransBaseType* data) {
    Int32 ret = 0;
    Int32 val = 0;
    Int32 len = sizeof(Int32);

    val = TRUE;
    ret = FileTool::pwriteFile(data->m_file_fd[ENUM_FILE_MAP],
        &val, len, data->m_finish_offset);
    if (0 == ret) {
        return 0;
    } else {
        return -1;
    } 
}

Void FileWriter::updateNextAck(TransBaseType* data) {
    list_node* pos = NULL;
    EvMsgTransData* pBlk = NULL; 
    
    list_for_each(pos, &m_list) {
        pBlk = MsgCenter::entry<EvMsgTransData>(pos);

        if (data->m_blk_next_ack >= pBlk->m_blk_beg) {
            if (data->m_blk_next_ack < pBlk->m_blk_end) {
                data->m_blk_next_ack = pBlk->m_blk_end;
            }
        } else {
            break;
        }
    }
}

Void FileWriter::moveBlk(TransBaseType* data) {
    Int32 pos = 0;
    Int32 size = 0;
    Int32 offset_1 = 0;
    Int32 offset_2 = 0;

    if (NULL == m_curr_blk) {
        return;
    }
    
    pos = m_blk_offset + m_blk_pos;
    offset_1 = ((pos - m_curr_blk->m_blk_beg) << DEF_FILE_UNIT_SHIFT_BITS);
    offset_2 = (m_blk_pos << DEF_FILE_UNIT_SHIFT_BITS); 
    
    if (m_curr_blk->m_blk_end <= m_blk_offset + DEF_MAX_BLK_CNT) {
        /* cp all now */
        size = FileTool::calcBlkSize(pos, m_curr_blk->m_blk_end, 
            data->m_file_size);
        memcpy(&m_buf[offset_2], &m_curr_blk->m_buf[offset_1], size); 

        m_blk_pos = m_curr_blk->m_blk_end - m_blk_offset;

        MsgCenter::freeMsg(m_curr_blk);
        m_curr_blk = NULL;
    } else {
        /* has data left */ 
        size = FileTool::calcBlkSize(pos, 
            m_blk_offset + DEF_MAX_BLK_CNT, data->m_file_size);
        memcpy(&m_buf[offset_2], &m_curr_blk->m_buf[offset_1], size);
        
        /* is full */
        m_blk_pos = DEF_MAX_BLK_CNT;
    } 
}

Int32 FileWriter::notifyRecv(EvMsgTransData* pReq, TransBaseType* data) {
    Int32 ret = 0;
    EvMsgTransDataAck* pRsp = NULL;
    
    pushBlk(pReq, data);
    ret = writeData(data);
    if (0 != ret) {
        return ret;
    }

    pRsp = MsgCenter::creatMsg<EvMsgTransDataAck>(CMD_TRANS_DATA_BLK_ACK);

    pRsp->m_time = pReq->m_time;
    pRsp->m_blk_next = data->m_blk_next_ack;

    ret = m_eng->transPkg(pRsp);
    if (0 != ret) {
        return ret;
    } 

    return 0;
}

Int32 FileWriter::dealBlkFinish(TransBaseType* data) {
    Int32 ret = 0;
    
    if (data->m_blk_end == data->m_blk_next_ack) {
        ret = writeData(data); 
        if (0 != ret) {
            return ret;
        }

        if (endOfFile(data)) {
            ret = setFinishFlag(data);
            if (0 != ret) {
                return ret;
            }

            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

Bool FileWriter::canWrite(TransBaseType* data) {
    if (DEF_MAX_BLK_CNT == m_blk_pos || (0 < m_blk_pos 
        && data->m_blk_end == m_blk_offset + m_blk_pos)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool FileWriter::endOfFile(TransBaseType* data) {
    if (data->m_blk_cnt == data->m_blk_next) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Int32 FileWriter::writeData(TransBaseType* data) {
    Int32 ret = 0;
    Bool bOk = TRUE;

    bOk = popBlk(data);
    while (bOk) { 
        moveBlk(data);

        if (canWrite(data)) { 
            ret = writeFile(data);
            if (0 != ret) {
                return -1;
            }
        } 

        bOk = popBlk(data);
    } 
    
    return 0;
}

Void FileWriter::pushBlk(EvMsgTransData* pReq, TransBaseType* data) {

    if (0 < pReq->m_buf_size && pReq->m_blk_end > data->m_blk_next_ack) {
        MsgCenter::add(pReq, &m_list);

        /* update the data blk can be written to disk */
        updateNextAck(data);
    } else {
        /* acked blk, ignore */
        MsgCenter::freeMsg(pReq);
    } 
}

Bool FileWriter::popBlk(TransBaseType*) {
    list_node* node = NULL;
    EvMsgTransData* pBlk = NULL;

    if (NULL == m_curr_blk) {
        while (!order_list_empty(&m_list)) {
            node = LIST_FIRST(&m_list); 
            
            pBlk = MsgCenter::entry<EvMsgTransData>(node);
            if (pBlk->m_blk_end <= m_blk_offset + m_blk_pos) {
                
                /* already be written blk */
                order_list_del(node, &m_list);
                MsgCenter::freeMsg(pBlk);
                continue;
            } else if (pBlk->m_blk_beg <= m_blk_offset + m_blk_pos) {
                /* suitable for writing */
                order_list_del(node, &m_list);
                m_curr_blk = pBlk;
                return TRUE;
            } else {
                /* has a gap blk */
                return FALSE;
            } 
        } 

        return FALSE;
    } else {
        return TRUE;
    }
}

Int32 FileWriter::writeFileMap(TransBaseType* data) {
    Int32 ret = 0;
    Int32 fd = -1;
    Int32 flag = FILE_FLAG_RD_WR;
    FileMapHeader ohd;
    Char szDataPath[MAX_FILEPATH_SIZE] = {0};
    Char szMapPath[MAX_FILEPATH_SIZE] = {0};

    do {        
        /* map file */
        snprintf(szMapPath, sizeof(szMapPath), "%s/%s.map", 
            data->m_file_path, data->m_file_id);
        fd = FileTool::openFile(szMapPath, flag);
        if (0 > fd) {
            ret = -1;
            break;
        }

        data->m_file_fd[ENUM_FILE_MAP] = fd;

        ret = FileTool::getMapHeader(fd, &ohd);
        if (0 != ret) {
            ret = -2;
            break;
        }

        if (0 != strcmp(ohd.m_file_id, data->m_file_id)) {
            ret = -3;
            break;
        }

        if (ohd.m_file_size != data->m_file_size
            || ohd.m_file_crc != data->m_file_crc
            || 0 != strcmp(ohd.m_file_name, data->m_file_name)) {
            ret = -3;
            break;
        } 

        /* prepare data file */
        snprintf(szDataPath, sizeof(szDataPath), "%s/%s",
            data->m_file_path, data->m_file_id);
            
        fd = FileTool::openFile(szDataPath, flag);
        if (0 <= fd) {
            data->m_file_fd[ENUM_FILE_DATA] = fd;
        } else {
            ret = -5;
            break;
        }

        data->m_blk_cnt = ohd.m_blk_cnt;
        data->m_blk_next = ohd.m_blk_next;
        data->m_completed = ohd.m_completed;
        
        data->m_blk_next_ack = data->m_blk_next;
        data->m_blk_beg = data->m_blk_next;
        data->m_blk_end = data->m_blk_cnt; 
        
        return 0;
    } while (0);

    /* if fail, then just close map and data files */
    if (0 <= data->m_file_fd[ENUM_FILE_MAP]) {
        FileTool::closeFd(&data->m_file_fd[ENUM_FILE_MAP]);
    }

    if (0 <= data->m_file_fd[ENUM_FILE_DATA]) {
        FileTool::closeFd(&data->m_file_fd[ENUM_FILE_DATA]);
    }

    return ret;
}

Int32 FileWriter::creatFileMap(TransBaseType* data) {
    Int32 ret = 0;
    Int32 fd = -1;
    FileMapHeader ohd;
    Char szDataPath[MAX_FILEPATH_SIZE] = {0};
    Char szMapPath[MAX_FILEPATH_SIZE] = {0};

    do {
        data->m_blk_cnt = FileTool::calcFileBlkCnt(data->m_file_size);
        data->m_blk_next = 0;
        data->m_completed = FALSE;
        
        data->m_blk_next_ack = data->m_blk_next;
        data->m_blk_beg = data->m_blk_next;
        data->m_blk_end = data->m_blk_cnt; 
        
        buildMapHeader(data, &ohd);
        
        snprintf(szMapPath, sizeof(szMapPath), "%s/%s.map",
            data->m_file_path, data->m_file_id);
        
        fd = FileTool::creatFile(szMapPath);
        if (0 > fd) {
            ret = -1;
            break;
        }

        data->m_file_fd[ENUM_FILE_MAP] = fd; 
        
        ret = FileTool::creatMapHeader(fd, &ohd);
        if (0 != ret) {
            ret = -2;
            break;
        }

        snprintf(szDataPath, sizeof(szDataPath), "%s/%s",
            data->m_file_path, data->m_file_id);
        
        fd = FileTool::creatFile(szDataPath);
        if (0 <= fd) {
            data->m_file_fd[ENUM_FILE_DATA] = fd;
        } else {
            ret = -3;
            break;
        } 
        
        return 0;
    } while (0);

    /* if fail, then delete map and data files */
    if (0 <= data->m_file_fd[ENUM_FILE_MAP]) {
        FileTool::closeFd(&data->m_file_fd[ENUM_FILE_MAP]);

        FileTool::delFile(szMapPath);
    }

    if (0 <= data->m_file_fd[ENUM_FILE_DATA]) {
        FileTool::closeFd(&data->m_file_fd[ENUM_FILE_DATA]);

        FileTool::delFile(szDataPath);
    }
    
    return ret;
}

Int32 FileWriter::prepareFileMap(TransBaseType* data) {
    Int32 ret = 0;
    Char szPath[MAX_FILEPATH_SIZE] = {0};

    snprintf(szPath, sizeof(szPath), "%s/%s.map", 
        data->m_file_path, data->m_file_id); 
    
    ret = FileTool::existsFile(szPath);
    if (0 == ret) {
        /* exists, write file map */
        ret = writeFileMap(data);
    } else if (1 == ret) {
        /* no exists */
        ret = creatFileMap(data);
    } else {
        ret = -1;
    } 
    
    return ret;
}
  
