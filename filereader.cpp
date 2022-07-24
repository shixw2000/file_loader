#include"filereader.h"
#include"msgcenter.h"
#include"filetool.h"
#include"engine.h"


static Int32 cmpWaitBlk(list_node* n1, list_node* n2) {
    EvMsgBlkRange* p1 = NULL;
    EvMsgBlkRange* p2 = NULL;

    p1 = MsgCenter::entry<EvMsgBlkRange>(n1);
    p2 = MsgCenter::entry<EvMsgBlkRange>(n2);

    if (p1->m_blk_end != p2->m_blk_end) {
        return p1->m_blk_end - p2->m_blk_end;
    } else if (p1->m_blk_beg != p2->m_blk_beg) {
        return p1->m_blk_beg - p2->m_blk_beg;
    } else {
        return p1->m_time - p2->m_time;
    }
}

static Int32 cmpReadBlk(list_node* n1, list_node* n2) {
    EvMsgBlkRange* p1 = NULL;
    EvMsgBlkRange* p2 = NULL;

    p1 = MsgCenter::entry<EvMsgBlkRange>(n1);
    p2 = MsgCenter::entry<EvMsgBlkRange>(n2);

    if (p1->m_blk_beg != p2->m_blk_beg) {
        return p1->m_blk_beg - p2->m_blk_beg;
    } else if (p1->m_blk_end != p2->m_blk_end) {
        return p1->m_blk_end - p2->m_blk_end;
    } else {
        return p1->m_time - p2->m_time;
    }
}


FileReader::FileReader(Engine* eng) {
    m_eng = eng;
    INIT_ORDER_LIST_HEAD(&m_list_read, &cmpReadBlk);
    INIT_ORDER_LIST_HEAD(&m_list_wait, &cmpWaitBlk);

    m_curr_blk = NULL;
    m_blk_offset = 0;
    m_blk_size = 0;
    m_blk_pos = 0;
}

FileReader::~FileReader() {
}

Int32 FileReader::start() {
    Int32 ret = 0;

    return ret;
}

Void FileReader::stop() {
    list_node* pos = NULL;
    list_node* n = NULL;
    EvMsgBlkRange* pBlk = NULL;

    /* clear unused data */
    list_for_each_safe(pos, n, &m_list_read) {
        pBlk = MsgCenter::entry<EvMsgBlkRange>(pos);

        order_list_del(pos, &m_list_read);
        MsgCenter::freeMsg(pBlk);
    }

    /* clear unused data */
    list_for_each_safe(pos, n, &m_list_wait) {
        pBlk = MsgCenter::entry<EvMsgBlkRange>(pos);

        order_list_del(pos, &m_list_wait);
        MsgCenter::freeMsg(pBlk);
    }

    MsgCenter::freeMsg(m_curr_blk);
    m_curr_blk = NULL;
}

Void FileReader::prepareSend(TransBaseType* data) {
    data->m_frame_size = 1;
    
    m_blk_offset = data->m_blk_next;
    m_blk_size = 0;
    m_blk_pos = 0; 
}

Int32 FileReader::notifySend(TransBaseType* data) {
    Int32 ret = 0;

    pushBlk(data); 
    ret = sendBlk(data);
    if (0 == ret) {
        return 0;
    } else {
        return -1;
    }
}

Int32 FileReader::grow(TransBaseType* data) {
    Int64 offset = 0;
    Int32 size = 0;
    Int32 ret = 0;
    Int32 next = 0;

    /* not finished, or end of file */
    if (m_blk_pos < m_blk_size 
        || m_blk_offset + m_blk_size >= data->m_blk_end) {
        return 0;
    }

    m_blk_offset += m_blk_size;
    m_blk_size = 0;
    m_blk_pos = 0;
        
    next = m_blk_offset + DEF_MAX_BLK_CNT;
    if (next > data->m_blk_end) {
        next = data->m_blk_end;
    }
        
    /* here size is > 0 */
    size = FileTool::calcBlkSize(m_blk_offset, next, data->m_file_size); 
        
    offset = m_blk_offset;
    offset <<= DEF_FILE_UNIT_SHIFT_BITS;
    
    ret = FileTool::preadFile(data->m_file_fd[ENUM_FILE_DATA],
        m_buf, size, offset);
    if (0 != ret) {
        return ret;
    } 

    m_blk_size = next - m_blk_offset; 
    return 0;
}

/* clear the acked blk from the wait queue */
Void FileReader::purgeWaitList(Int32 ackBlk, TransBaseType* data) {
    list_node* pos = NULL;
    list_node* n = NULL;
    EvMsgBlkRange* pBlk = NULL; 

    if (data->m_blk_next_ack < ackBlk) { 
        list_for_each_safe(pos, n, &m_list_wait) {
            pBlk = MsgCenter::entry<EvMsgBlkRange>(pos);

            if (pBlk->m_blk_end <= ackBlk) {

                /* remove acked blk */
                order_list_del(pos, &m_list_wait);
                MsgCenter::freeMsg(pBlk);
            } else {
                break;
            }
        }

        data->m_blk_next_ack = ackBlk;

        /* here may not enter in normal case */
        if (data->m_blk_next < data->m_blk_next_ack) {
            data->m_blk_next = data->m_blk_next_ack;
        } 
    }
}

Void FileReader::pushBlk(TransBaseType* data) {
    if (data->m_blk_next < data->m_blk_end) { 
        EvMsgBlkRange* pMsg = NULL; 

        pMsg = MsgCenter::creatMsg<EvMsgBlkRange>(CMD_DATA_BLK_RANGE);

        pMsg->m_time = m_eng->now();
        pMsg->m_blk_beg = data->m_blk_next;
        pMsg->m_blk_end = pMsg->m_blk_beg + data->m_frame_size;
        
        if (pMsg->m_blk_end > data->m_blk_end) {
            pMsg->m_blk_end = data->m_blk_end;
        }

        data->m_blk_next = pMsg->m_blk_end;
        
        MsgCenter::add(pMsg, &m_list_read); 
    } 
}

Bool FileReader::popBlk(TransBaseType* data) {
    Int32 size = 0;
    Bool bOk = TRUE;
    list_node* first = NULL;
    EvMsgBlkRange* pReq = NULL;
    
    if (NULL == m_curr_blk) {
        while (!order_list_empty(&m_list_read)) { 

            first = LIST_FIRST(&m_list_read);
            order_list_del(first, &m_list_read);

            pReq = MsgCenter::entry<EvMsgBlkRange>(first); 
            bOk = FileTool::chkBlkValid(pReq->m_blk_beg,
                pReq->m_blk_end,
                data->m_blk_beg,
                data->m_blk_end);
            if (!bOk || pReq->m_blk_beg != m_blk_offset + m_blk_pos) {
                /* invalid range */
                MsgCenter::freeMsg(pReq);
                continue;
            }

            size = FileTool::calcBlkSize(pReq->m_blk_beg, 
                pReq->m_blk_end, data->m_file_size);
            if (0 >= size) {
                /* no data, ignore */
                MsgCenter::freeMsg(pReq);
                continue;
            }
            
            /* get a new blk range */
            m_curr_blk = MsgCenter::creatMsg<EvMsgTransData>(
                CMD_TRANS_DATA_BLK, size);
            m_curr_blk->m_blk_beg = pReq->m_blk_beg;
            m_curr_blk->m_blk_end = pReq->m_blk_end;
            m_curr_blk->m_buf_size = size;

            /* wait for ack later */
            MsgCenter::add(pReq, &m_list_wait); 
            
            return TRUE;
        } 

        return FALSE;
    } else {
        return TRUE;
    }
}

Bool FileReader::fillBlk(TransBaseType* data) {
    Int32 leftLen = 0;
    Int32 offset_1 = 0;
    Int32 offset_2 = 0;
    
    offset_1 = m_blk_offset + m_blk_pos - m_curr_blk->m_blk_beg;
    offset_1 <<= DEF_FILE_UNIT_SHIFT_BITS;
    offset_2 = (m_blk_pos << DEF_FILE_UNIT_SHIFT_BITS);
    
    if (m_blk_offset + m_blk_size >= m_curr_blk->m_blk_end) {
        leftLen = FileTool::calcBlkSize(
            m_blk_offset + m_blk_pos, 
            m_curr_blk->m_blk_end, data->m_file_size); 
        
        memcpy(&m_curr_blk->m_buf[offset_1], &m_buf[offset_2], leftLen);
        m_blk_pos = m_curr_blk->m_blk_end - m_blk_offset; 

        return TRUE;
    } else {
        /* msg not completed, read next time */
        leftLen = FileTool::calcBlkSize(
            m_blk_offset + m_blk_pos, 
            m_blk_offset + m_blk_size, data->m_file_size);

        memcpy(&m_curr_blk->m_buf[offset_1], &m_buf[offset_2], leftLen);
        m_blk_pos = m_blk_size;

        return FALSE;
    } 
}

/* return: 1: ok, 0: empt, -1: error */
Int32 FileReader::sendBlk(TransBaseType* data) {
    Int32 ret = 0;
    Bool bOk = TRUE;
 
    bOk = popBlk(data);
    while (bOk) {
        ret = grow(data);
        if (0 != ret) {
            return -1;
        }

        bOk = fillBlk(data);
        if (bOk) {
            /* fulfill a blk, then send it */
            ret = m_eng->transPkg(m_curr_blk);
            m_curr_blk = NULL;

            if (0 == ret) { 
                /* try next blk */
                bOk = popBlk(data);
            } else {
                return ret;
            }
        } else {
            /* uncompleted blk, grwo to get more */
            bOk = TRUE;
        }
    }
    
    return 0;
}

Int32 FileReader::parseRawFile(TransBaseType* data) {
    Int32 ret = 0;
    Char szPath[MAX_FILEPATH_SIZE] = {0};
 
    snprintf(szPath, sizeof(szPath), "%s/%s", 
        data->m_file_path, data->m_file_name);

    do {
        ret = parseFile(szPath, data);
        if (0 != ret) {
            break;
        }

        /* check file crc */
        if (data->m_file_crc != 0) {
            ret = -3;
            break;
        }

        data->m_blk_cnt = FileTool::calcFileBlkCnt(data->m_file_size);
    } while (0);
    
    return ret;
}
    

Int32 FileReader::parseFile(const Char path[], TransBaseType* data) {
    Int32 ret = 0;
    Int32 fd = -1;
    FileInfoType info;

    do { 
        ret = FileTool::statFile(path, &info);
        if (0 != ret) {
            ret = -1;
            break;
        }

        if (ENUM_FILE_TYPE_REG != info.m_file_type) {
            ret = -2;
            break;
        }

        /* check file size */
        if (data->m_file_size != info.m_file_size) {
            ret = -3;
            break;
        } 

        fd = FileTool::openFile(path, FILE_FLAG_RD);
        if (0 > fd) {
            ret = -3;
            break;
        } 
        
        data->m_file_fd[ENUM_FILE_DATA] = fd;

        ret = 0;
    } while (0);

    return ret;
}

Int32 FileReader::parseMapFile(TransBaseType* data) {
    Int32 ret = 0;
    Int32 fd = -1;
    Int32 flag = FILE_FLAG_RD;
    FileMapHeader ohd;
    Char szDataPath[MAX_FILEPATH_SIZE] = {0};
    Char szMapPath[MAX_FILEPATH_SIZE] = {0};

    do {
        /* read map file */
        snprintf(szMapPath, sizeof(szMapPath), "%s/%s.map", 
            data->m_file_path, data->m_file_id);
        fd = FileTool::openFile(szMapPath, flag);
        if (0 > fd) {
            ret = -1;
            break;
        }

        /* no need map fd later */
        ret = FileTool::getMapHeader(fd, &ohd);
        FileTool::closeFd(&fd);
        if (0 != ret) { 
            ret = -2;
            break;
        }

        if (0 != strcmp(ohd.m_file_id, data->m_file_id) 
            || !ohd.m_completed) {
            ret = -3;
            break;
        } 

        data->m_file_size = ohd.m_file_size;
        data->m_file_crc = ohd.m_file_crc;
        data->m_blk_cnt = ohd.m_blk_cnt;
        strncpy(data->m_file_name, ohd.m_file_name, MAX_FILENAME_SIZE);

        snprintf(szDataPath, sizeof(szDataPath), "%s/%s",
            data->m_file_path, data->m_file_id);

        /* prepare data file to read */
        ret = parseFile(szDataPath, data);
        if (0 != ret) {
            ret = -4;
            break;
        } 

        if (data->m_file_crc != 0) {
            ret = -5;
            break;
        }

        ret = 0;
    } while (0);

    return ret;
} 

