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
        /* must not compare timer, for timer may be update by retrans */
        return 0;
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
        if (p1->m_time < p2->m_time) {
            return -1;
        } else if (p1->m_time > p2->m_time) {
            return 1;
        } else {
            return 0;
        }
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
}

Int32 FileReader::waitSize() const {
    return LIST_SIZE(&m_list_wait) + LIST_SIZE(&m_list_read);
}

EvMsgBlkRange* FileReader::getLastWait() {
    list_node* first = NULL;
    EvMsgBlkRange* pReq = NULL;
    
    if (!order_list_empty(&m_list_wait)) { 
        first = LIST_FIRST(&m_list_wait);
        pReq = MsgCenter::entry<EvMsgBlkRange>(first); 

        return pReq;
    } else {
        return NULL;
    }
}

Int32 FileReader::retransFrame(const EvMsgBlkRange* pBlk, TransBaseType* data) {
    Int64 offset = 0;
    Int32 ret = 0;
    Int32 size = 0;
    Int32 beg = pBlk->m_blk_beg;
    Int32 end = pBlk->m_blk_end;
    Uint32 now = 0;
    EvMsgTransData* pMsg = NULL;
    
    now = m_eng->now();
    
    size = FileTool::calcBlkSize(beg, end, data->m_file_size); 
    pMsg = MsgCenter::creatMsg<EvMsgTransData>(CMD_TRANS_DATA_BLK, size);
    pMsg->m_blk_beg = beg;
    pMsg->m_blk_end = end;
    pMsg->m_time = now;
    pMsg->m_buf_size = size;

    /* the whole block is inside cache */
    if (beg >= m_blk_offset && end <= m_blk_offset + m_blk_size) {
        offset = beg - m_blk_offset;
        offset <<= DEF_FILE_UNIT_SHIFT_BITS;
        memcpy(pMsg->m_buf, &m_buf[offset], size); 
    } else {
        /* here must be Int64 */
        offset = beg;
        offset <<= DEF_FILE_UNIT_SHIFT_BITS;
        /* read from file */
        ret = FileTool::preadFile(data->m_file_fd[ENUM_FILE_DATA],
            pMsg->m_buf, size, offset);
        if (0 != ret) {
            MsgCenter::freeMsg(pMsg);
            return -1;
        }
    }

    ret = m_eng->transPkg(pMsg);
    if (0 == ret) {
        return 0;
    } else {
        return -1;
    }
}

Void FileReader::prepareSend(TransBaseType* data) { 
    m_blk_offset = data->m_blk_next;
    m_blk_size = 0;
    m_blk_pos = 0; 
}

Void FileReader::postSend(TransBaseType* data) { 
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

    m_blk_offset = 0;
    m_blk_size = 0;
    m_blk_pos = 0; 

    closeTransData(data);
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
Void FileReader::purgeWaitList(const EvMsgTransDataAck* pRsp, 
    TransBaseType* data) {
    list_node* pos = NULL;
    list_node* n = NULL;
    EvMsgBlkRange* pBlk = NULL; 
    Int32 ackBlk = pRsp->m_blk_next;

    if (data->m_blk_next_ack < ackBlk) { 
        list_for_each_safe(pos, n, &m_list_wait) {
            pBlk = MsgCenter::entry<EvMsgBlkRange>(pos);

            if (pBlk->m_blk_end <= ackBlk) {
                LOG_DEBUG("purge| beg=%d| end=%d| ack=%d| time=%u->%u|\n",
                    pBlk->m_blk_beg,
                    pBlk->m_blk_end,
                    ackBlk,
                    pRsp->m_time,
                    m_eng->now());

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

Bool FileReader::pushBlk(TransBaseType* data) {    
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

        return TRUE;
    } else {
        return FALSE;
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
            m_curr_blk->m_time = pReq->m_time;
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
            Int32 beg = m_curr_blk->m_blk_beg;
            Int32 end = m_curr_blk->m_blk_end;
            Int32 size = m_curr_blk->m_buf_size;
            Uint32 time = m_curr_blk->m_time;
            
            LOG_INFO("send_blk| beg=%d| end=%d| size=%d| time=%u|",
                beg, end, size, time);
                    
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
 
    snprintf(szPath, sizeof(szPath), "%.128s/%.100s", 
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
        snprintf(szMapPath, sizeof(szMapPath), "%.128s/%.100s.map", 
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

        snprintf(szDataPath, sizeof(szDataPath), "%.128s/%.100s",
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

