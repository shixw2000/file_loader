#include"queworker.h"
#include"msgcenter.h"


QueWorker::QueWorker() {
    m_running = FALSE;
    INIT_LIST_HEAD(&m_queue);

    m_free_cnt = 0;
    m_deal_cnt = 0;
}

Int32 QueWorker::start() {
    m_running = TRUE;
    return 0;
}

Void QueWorker::stop() {
    m_running = FALSE;
}

Bool QueWorker::notify(Void* msg) {    
    if (NULL != msg) {
        if (m_running) {
            ++m_deal_cnt;
            MsgCenter::notify(msg, &m_queue);

            return TRUE;
        } else {
            ++m_free_cnt;
            MsgCenter::freeMsg(msg);

            return FALSE;
        }
    } else {
        return FALSE;
    }
}

Bool QueWorker::emerge(Void* msg) {
    if (NULL != msg) {
        if (m_running) {
            ++m_deal_cnt;
            MsgCenter::emerge(msg, &m_queue);

            return TRUE;
        } else {
            ++m_free_cnt;
            MsgCenter::freeMsg(msg);

            return FALSE;
        }
    } else {
        return FALSE;
    }
}

Bool QueWorker::doWork() {
    Bool done = FALSE;
    list_node* node = NULL;
    Void* msg = NULL;

    if (!list_empty(&m_queue)) {
        node = LIST_FIRST(&m_queue);

        list_del(node, &m_queue);
        
        msg = MsgCenter::from(node);
    }

    if (NULL != msg) { 
        if (m_running) {
            dealEvent(msg);
        } else {
            MsgCenter::freeMsg(msg);
        }

        done = TRUE;
    }

    return done;
}

Void QueWorker::consume() {
    Bool done = TRUE;

    do {
        done = doWork();
    } while (done);
}

