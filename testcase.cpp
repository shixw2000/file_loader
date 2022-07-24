#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include"globaltype.h"
#include"cmdctx.h"
#include"ticktimer.h"
#include"testcase.h"
#include"listnode.h"
#include"cthread.h"
#include"fdcmdctx.h"
#include"fdtimer.h"


int test_ctx(int opt) {
    int ret = 0;
    FdCmdCtx* ctx1 = NULL;
    FdCmdCtx* ctx2 = NULL;
    FdTimer* timer = NULL;
    CThread* thread = NULL;

    I_NEW(FdCmdCtx, ctx1);
    I_NEW(FdCmdCtx, ctx2);

    I_NEW(FdTimer, timer);
    I_NEW(CThread, thread);

    thread->addEle(timer);

    ctx1->setTimer(timer);
    thread->addEle(ctx1);

    ctx2->setTimer(timer);
    thread->addEle(ctx2);
    
    ret = ctx1->start();
    ret = ctx2->start();
    ret = timer->start();
    ret = thread->start();

    ctx1->setTest(ctx2);
    ctx2->setTest(ctx1);

    if (1 == opt) {
        ctx1->test_01();
    } else if (2 == opt) {
        ctx1->test_02();
    } else {
        return -1;
    }

    thread->run();

    thread->stop();
    timer->stop();
    ctx1->stop();
    ctx2->stop();
    return 0;
}


struct NumNode {
    list_node m_node;
    int m_num;
};

static int cmpNode(list_node* n1, list_node* n2) {
    NumNode* p1 = list_entry(n1, NumNode, m_node);
    NumNode* p2 = list_entry(n2, NumNode, m_node);
    
    return p1->m_num - p2->m_num;
}

int test_node() {
    int ret = 0;
    int val = 0;
    order_list_head list;
    NumNode* pn = NULL;
    list_node* pos = NULL;

    INIT_ORDER_LIST_HEAD(&list, &cmpNode);

    LOG_INFO("Enter a num:");
    scanf("%d", &val);
    while (0 != val) {
        I_NEW(NumNode, pn);
        INIT_LIST_NODE(&pn->m_node);
        pn->m_num = val;

        order_list_add(&pn->m_node, &list);

        LOG_INFO("Enter a num:");
        scanf("%d", &val);
    }

    LOG_INFO("list size=%d|", LIST_SIZE(&list));
    
    list_for_each(pos, &list) {
        pn = list_entry(pos, NumNode, m_node);
        
        printf("%d ", pn->m_num);
    }

    printf("\n");

    LOG_INFO("Enter a num to search:");
    scanf("%d", &val);

    I_NEW(NumNode, pn);
    INIT_LIST_NODE(&pn->m_node);
    pn->m_num = val;

    pos = order_list_find(&pn->m_node, &list);
    if (NULL != pos) {
        pn = list_entry(pos, NumNode, m_node);
        
        LOG_INFO("Found: %d|", pn->m_num);
    } else {
        LOG_INFO("Not found");
    }

    return ret;
}


int test_main(int opt) {
    int ret = 0;

    if (1 == opt || opt == 2) {
        ret = test_ctx(opt);
    } else if (3 == opt) {
        test_node();
    } else {
    }

    return ret;
}
