#ifndef __QUEWORKER_H__
#define __QUEWORKER_H__
/****
    * This is a Queue supported by back_push and front push.
    * It pops each msg one by one, dealing the msgs  if running;
    * otherwise, it just free the messages in the queue.
****/
#include"globaltype.h"
#include"listnode.h"


class QueWorker {
public:
    QueWorker();
    
    virtual ~QueWorker() {}

    /* this is a turn on switch for the queue,
            It must be called before any others.
            You can override this funcion in subclasses */
    virtual Int32 start();

    /* this is a turn off switch for the queue.
            It must be called after a end usage.
            If a queue has been stop, then all msgs
            pushing is just deleted, and all of msgs
            still the on the queue will be just deleted
            without a handler. */
    virtual Void stop();
    
    /* normal push back one by one, as a fifo principle */
    virtual Bool notify(Void* msg);

    /* this is a urgent method for pushing msg in front of the queue */
    virtual Bool emerge(Void* msg);
    
    /* pop the front msg and try to handling */
    Bool doWork();

    /* exhausts all of msg on the queue */
    void consume();
    
protected:
    virtual Void dealEvent(Void* msg) = 0;
    
private:
    Uint64 m_free_cnt;  // total of deleted msgs
    Uint64 m_deal_cnt;  // total of handled msgs
    Bool m_running;
    list_head m_queue;
};

#endif

