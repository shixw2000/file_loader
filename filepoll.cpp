#include<string.h>
#include<poll.h>
#include<unistd.h>
#include"filepoll.h"
#include"msgcenter.h"
#include"sockutil.h"


static const Int32 ERR_FLAG = POLLERR | POLLHUP | POLLNVAL;
static const Int32 DEF_ITEM_CAPACITY = 1000;


FilePoll::FilePoll() : m_capacity(DEF_ITEM_CAPACITY) {
    m_fds = NULL;
    m_items = NULL;
    m_fast_round = FALSE;
}

FilePoll::~FilePoll() {
}

Int32 FilePoll::init() {
    Int32 ret = 0;

    ARR_NEW(struct pollfd, m_capacity, m_fds);
    memset(m_fds, 0, sizeof(struct pollfd) * m_capacity);
    
    ARR_NEW(PollItem, m_capacity, m_items);
    for (int i=0; i<m_capacity; ++i) {
        initItem(&m_items[i]);
    }
    
    INIT_LIST_HEAD(&m_run_list);
    INIT_LIST_HEAD(&m_wait_list);
    INIT_LIST_HEAD(&m_idle_list);

    return ret;
}

Void FilePoll::finish() {
    if (NULL != m_items) {
        for (int i=0; i<m_capacity; ++i) {
            finishItem(&m_items[i]);
        }

        ARR_FREE(m_items);
    }

    if (NULL != m_fds) { 
        ARR_FREE(m_fds);
    }

    INIT_LIST_HEAD(&m_run_list);
    INIT_LIST_HEAD(&m_wait_list);
    INIT_LIST_HEAD(&m_idle_list);

    return;
}

Int32 FilePoll::chkEvent() { 
    struct pollfd* pfd = NULL;
    PollItem* pos = NULL;
    Int32 ret = 0;
    Int32 size = 0; 
    Int32 timeout = 1000; 
    
    m_fast_round = FALSE;
    
    if (!list_empty(&m_wait_list)) {
        size = 0;
        list_for_each_entry(pos, &m_wait_list, m_node) {
            pfd = &m_fds[size++]; 
            
            pfd->fd = pos->m_fd;
            pfd->events = 0; 
            pfd->revents = 0;
            
            if (!pos->m_more_read && pos->m_test_read) {
                pfd->events |= POLLIN;
            }
            
            if (!pos->m_can_write) {
                pfd->events |= POLLOUT;
            }

            if (pos->m_more_read || (pos->m_more_write && pos->m_can_write)) {
                if (!m_fast_round) {
                    m_fast_round = TRUE;
                }
            }
        }

        /* if has something left, then do a fast check */
        if (m_fast_round) {
            timeout = 0;
        }
        
        ret = poll(m_fds, size, timeout);
        if (0 < ret || m_fast_round) {
            for (int i=0; i<size; ++i) {
                pfd = &m_fds[i]; 
                pos = &m_items[pfd->fd];

                if (!pos->m_more_read && (POLLIN & pfd->revents)) {
                    pos->m_more_read = TRUE;
                }

                if (POLLOUT & pfd->revents) { 
                    if (!pos->m_can_write) {
                        pos->m_can_write = TRUE;
                    }
                }

                if (ERR_FLAG & pfd->revents) {
                    LOG_INFO("chk_event_poll| fd=%d| revent=0x%x|"
                        " error=poll error|",
                        pfd->fd, pfd->revents);
                    
                    pos->m_error = TRUE;
                }

                if (pos->m_more_read || pos->m_error
                    || (pos->m_can_write && pos->m_more_write)) {
                    list_del(&pos->m_node, &m_wait_list);
                    list_add_back(&pos->m_node, &m_run_list);

                    pos->m_status = ENUM_POLL_RUNNING;
                }
            }

            ret = 0;
        } else if (0 == ret || EINTR == errno) {
            ret = 0;
        } else {
            ret = -1;
        }
    }

    return ret;
}

Int32 FilePoll::addSendQue(Int32 fd, Void* msg) {
    PollItem* pos = NULL;

    pos = &m_items[fd];
    if (pos->m_fd == fd) {
        MsgCenter::notify(msg, &pos->m_snd_msgs);

        if (!pos->m_more_write) {
            pos->m_more_write = TRUE; 

            if (pos->m_status == ENUM_POLL_IDLE) {

                list_del(&pos->m_node, &m_idle_list);
                list_add_back(&pos->m_node, &m_wait_list);
                
                pos->m_status = ENUM_POLL_WAIT;
            }
        }

        return 0;
    } else {
        LOG_INFO("addMsg| fd=%d| error=fd is invalid|", fd);

        MsgCenter::freeMsg(msg);
        return -1;
    }
}

Void FilePoll::dealItem(PollItem* item) {
    Int32 ret = 0;

    if (item->m_can_write && item->m_more_write) {
        ret = item->m_oper->writeFd(item);
        if (0 != ret) {
            /* if send error, mark it */
            if (!item->m_error) {
                item->m_error = TRUE;
            }
        }
    }

    if (item->m_more_read) {
        item->m_more_read = FALSE;
       
        ret = item->m_oper->readFd(item);
        if (0 != ret) {
            /* mark an erro */
            if (!item->m_error) {
                item->m_error = TRUE;
            }
        } 

        if (!list_empty(&item->m_rcv_msgs)) {
            item->m_oper->dealFd(item);
        }
    }

    /* a normal fd */
    if (!item->m_error) {
        if (item->m_more_read || item->m_test_read 
            || item->m_more_write || !item->m_can_write) { 
            list_del(&item->m_node, &m_run_list);
            list_add_back(&item->m_node, &m_wait_list);
            item->m_status = ENUM_POLL_WAIT; 
        } else {
            list_del(&item->m_node, &m_run_list); 
            list_add_back(&item->m_node, &m_idle_list);
            item->m_status = ENUM_POLL_IDLE;
        } 
    } else {
        /* a error fd */
        finishItem(item);
    }

    return;
}

Int32 FilePoll::run() {
    PollItem* pos = NULL;
    PollItem* n = NULL;
    Int32 ret = 0;

    ret = chkEvent();
    if (0 == ret && !list_empty(&m_run_list)) {
        list_for_each_entry_safe(pos, n, &m_run_list, m_node) { 
            dealItem(pos); 
        }
    } 

    return ret;
}

Void FilePoll::initItem(PollItem* item) {
    memset(item, 0, sizeof(*item));

    item->m_fd = -1;
    item->m_status = ENUM_POLL_STATUS_NULL;

    item->m_oper = NULL;
    item->m_more_read = FALSE;
    item->m_test_read = FALSE;
    item->m_more_write = FALSE;
    item->m_can_write = FALSE;
    item->m_error = FALSE;
    item->m_del_oper = FALSE;
    INIT_LIST_HEAD(&item->m_snd_msgs);
    INIT_LIST_HEAD(&item->m_rcv_msgs);
    INIT_LIST_NODE(&item->m_node);
}

Void FilePoll::finishItem(PollItem* item) {
    list_node* pos = NULL;
    list_node* n = NULL;
    Void* msg = NULL;
    Int32 fd = -1; // use by the end 

    if (0 > item->m_fd) {
        return;
    } else {
        fd = item->m_fd;
        item->m_fd = -1;
    }

    LOG_INFO("finish_item| fd=%d|", fd);

    if (ENUM_POLL_IDLE == item->m_status) {
        list_del(&item->m_node, &m_idle_list);
    } else if (ENUM_POLL_WAIT == item->m_status) {
        list_del(&item->m_node, &m_wait_list);
    } else if (ENUM_POLL_RUNNING == item->m_status) {
        list_del(&item->m_node, &m_run_list);
    } else {
    } 
    
    list_for_each_safe(pos, n, &item->m_snd_msgs) {
        list_del(pos, &item->m_snd_msgs);
                
        msg = MsgCenter::entry<Void>(pos);
        MsgCenter::freeMsg(msg);
    }

    list_for_each_safe(pos, n, &item->m_rcv_msgs) {
        list_del(pos, &item->m_rcv_msgs);
                
        msg = MsgCenter::entry<Void>(pos);
        MsgCenter::freeMsg(msg);
    }
    
    item->m_status = ENUM_POLL_STATUS_NULL;
    item->m_more_read = FALSE;
    item->m_test_read = FALSE;
    item->m_more_write = FALSE;
    item->m_can_write = FALSE;
    item->m_error = FALSE; 

    if (NULL != item->m_oper) {        
        item->m_oper->finish();

        if (item->m_del_oper) {
            I_FREE(item->m_oper);
        }

        item->m_oper = NULL;
    } 

    if (item->m_del_oper) {
        closeHd(fd);
    }
    
    item->m_del_oper = FALSE;
}

Int32 FilePoll::addOper(Int32 fd, I_FdOper* oper, 
    Bool bDel, Bool bRd, Bool bWr) {
    PollItem* item = NULL;

    if (0 <= fd && fd < m_capacity && -1 == m_items[fd].m_fd) {
        item = &m_items[fd];
        
        initItem(item);
        
        item->m_fd = fd;
        item->m_oper = oper;
        item->m_del_oper = bDel;
        
        list_add_back(&item->m_node, &m_idle_list);
        item->m_status = ENUM_POLL_IDLE; 

        if (bRd) {
            startRead(fd);
        }

        if (!bWr) {
            stopWrite(fd);
        }

        return 0;
    } else {
        oper->finish();

        if (bDel) {
            I_FREE(oper);

            closeHd(fd); 
        } 
        
        return -1;
    }
}

Void FilePoll::startRead(Int32 fd) {
    PollItem* item = NULL;

    item = &m_items[fd];
    if (fd == item->m_fd) {
        if (!item->m_test_read) {
            item->m_test_read = TRUE;

            if (ENUM_POLL_IDLE == item->m_status) {
                list_del(&item->m_node, &m_idle_list);    
                list_add_back(&item->m_node, &m_wait_list);
                item->m_status = ENUM_POLL_WAIT;
            }
        }
    }
}

Void FilePoll::stopWrite(Int32 fd) {
    PollItem* item = NULL;

    item = &m_items[fd];
    if (fd == item->m_fd) {
        if (!item->m_can_write) {
            item->m_can_write = TRUE;
        }
    }
}

Int32 FilePoll::delOper(Int32 fd) {
    Int32 ret = 0;
    PollItem* item = NULL;

    if (0 <= fd && fd < m_capacity) {
        item = &m_items[fd];
        if (fd == item->m_fd) {
            finishItem(item);
        } else {
            ret = -1;
        }
    } else {
        ret = -1;
    }

    return ret;
}

