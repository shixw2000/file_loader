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


class MsgCenter { 
public: 

    template<typename T>
    static T* creatMsg(Int32 type, Int32 extlen = 0) {
        Void* msg = NULL;
        Int32 total = sizeof(T) + extlen;

        msg = creat(type, total); 
        return cast<T>(msg);
    }

    template<typename T>
    static T* cast(Void* msg) {
        return reinterpret_cast<T*>(msg);
    }

    template<typename T>
    static T* entry(list_node* node) {
        return reinterpret_cast<T*>(from(node));
    }
    
    static Void* from(list_node* node);

    static void freeMsg(Void* msg);
    
    static Int32 cmd(const Void* msg);

    static Int32 error(const Void* msg);

    static Void setError(Int32, Void* msg);

    static const Char* ID(const Void* msg);
    static Void setID(const Char ID[], Void* msg);
    
    static void notify(Void* msg, list_head*);
    static void emerge(Void* msg, list_head*);

    static void add(Void* msg, order_list_head* head); 

    static Uint32 random();
    static Uint64 clock();

private:
    static Void* creat(Int32 type, Int32 extlen);
    static list_node* to(Void* msg);
    
};

#endif

