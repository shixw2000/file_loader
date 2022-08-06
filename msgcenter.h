#ifndef __MSGCENTER_H__
#define __MSGCENTER_H__
/****
    * This file is used a message center.
    * most of operations with msgs are defined here.
    * A standard msg is composed of a list_node,
    * a msg header, and the msg body.
****/
#include<string.h>
#include"globaltype.h"
#include"listnode.h"
#include"ievent.h"


class MsgCenter { 
    struct _internal;
    
public: 

    template<typename T>
    static T* creatMsg(Int32 type, Int32 extlen = 0) {
        Void* msg = NULL;
        Int32 len = sizeof(T) + extlen;

        msg = creat(type, len); 
        return cast<T>(msg);
    }

    template<typename T>
    static T* cast(Void* msg) {
        return reinterpret_cast<T*>(msg);
    }

    template<typename T>
    static T* entry(list_node* node) {
        return cast<T>(node2msg(node));
    } 

    static Void* prepend(Int32 len);
    static Int32 fillMsg(const Void* buf, Int32 len, Void* msg);
    static Bool endOfMsg(const Void* msg);
    static Void reopen(Void* msg);
    
    static Void* dupUdp(const Void* buf, Int32 len);
    static void freeMsg(Void* msg);

    static Char* buffer(Void* msg);
    static Int32 bufsize(Void* msg);
    static Int32 bufpos(Void* msg);
    static Void setbufpos(Int32 pos, Void* msg);
    
    static Int32 cmd(const Void* msg);

    static Int32 error(const Void* msg);

    static Void setError(Int32, Void* msg);

    static const Char* ID(const Void* msg);
    static Void setID(const Char ID[], Void* msg);
    
    static void notify(Void* msg, list_head*);
    static void emerge(Void* msg, list_head*);

    static void add(Void* msg, order_list_head* head);     

private:
    static Void* creat(Int32 type, Int32 extlen);
    
    static Void* node2msg(list_node* node);
    
    static list_node* msg2node(Void* msg);
    
    static _internal* msg2intern(Void* msg);
    static const _internal* msg2intern(const Void* msg);
    
    static EvMsgHeader* msg2head(Void* msg);
    static const EvMsgHeader* msg2head(const Void* msg);
    
};

#endif

