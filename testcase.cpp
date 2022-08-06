#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include"globaltype.h"
#include"cmdctx.h"
#include"ticktimer.h"
#include"testcase.h"
#include"listnode.h"
#include"cthread.h"
#include"sockutil.h"
#include"filepoll.h"
#include"filetool.h"
#include"eventmsg.h"
#include"msgcenter.h"
#include"udpoper.h"
#include"stdinoper.h"
#include"tcpoper.h"
#include"tcpcmdctx.h"
#include"timeroper.h"
#include"eventoper.h"


extern Void* test_upload();
extern Void* test_download();

Void ProcHandler::process(Void* msg) {
    Int32 cmd = 0;
    EvMsgTransData* pReq = NULL;

    cmd = MsgCenter::cmd(msg); 
    if (CMD_TRANS_DATA_BLK == cmd) {
        pReq = MsgCenter::cast<EvMsgTransData>(msg); 

        LOG_INFO("size=%d| time=%d| msg=%s|",
            pReq->m_buf_size, pReq->m_time, pReq->m_buf);
    }
    
    MsgCenter::freeMsg(msg);
}

int test_ctx() {
    int ret = 0;
    
    return ret;
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

void test_udp() {
    Int32 ret = 0;
    Int32 fd = -1;
    Int32 peer_addrlen = 0;
    int local_port = 9999;
    int peer_port = 9998;
    Char local_ip[] = "172.8.1.12"; 
    Char peer_ip[] = "172.8.1.12";
    Char peer_addr[DEF_ADDR_SIZE] = {0};
    FilePoll* fp = NULL;
    ProcHandler* hd = NULL;
    UdpOper* msgoper = NULL;
    StdinOper* stdinoper = NULL;

    LOG_INFO("Enter local port and peer port:");
    scanf("%d %d", &local_port, &peer_port);

    do {
        fd = creatUdpSock(local_ip, local_port);
        if (0 > fd) {
            break;
        }

        peer_addrlen = DEF_ADDR_SIZE;
        ret = creatAddr(peer_ip, peer_port, peer_addr, &peer_addrlen);
        if (0 != ret) {
            break;
        }
        
        I_NEW(FilePoll, fp);
        ret = fp->init();
        if (0 != ret) {
            break;
        }
        
        I_NEW(ProcHandler, hd);
        
        I_NEW(UdpOper, msgoper); 
        msgoper->setParam(hd, peer_addr, peer_addrlen);
        ret = fp->addOper(fd, msgoper, TRUE, TRUE, TRUE);
        if (0 != ret) {
            break;
        }
        
        I_NEW(StdinOper, stdinoper);
        stdinoper->setParam(fd, fp);
        ret = fp->addOper(0, stdinoper, TRUE, TRUE, FALSE);
        if (0 != ret) {
            break;
        }

        while (TRUE) {
            fp->run();
        }
    } while (0);

    if (NULL != fp) {
        fp->finish();
        I_FREE(fp);
    }
    
    return;
}

void test_tcp_server() {
    Int32 ret = 0;
    Int32 fd = -1;
    int local_port = 9999;
    Char local_ip[32] = "172.8.1.12"; 
    FilePoll* fp = NULL;
    ProcHandler* hd = NULL;
    TcpSrvTest* msgoper = NULL;
    
    LOG_INFO("Enter server ip and server port:");
    scanf("%s %d", local_ip, &local_port);

    do {
        fd = creatTcpSrv(local_ip, local_port);
        if (0 > fd) {
            break;
        }

        I_NEW(FilePoll, fp);
        ret = fp->init();
        if (0 != ret) {
            break;
        }

        I_NEW(ProcHandler, hd);
        
        I_NEW(TcpSrvTest, msgoper); 
        msgoper->setParam(fp, hd);
        ret = fp->addOper(fd, msgoper, TRUE, TRUE, FALSE);
        if (0 != ret) {
            break;
        }

        while (TRUE) {
            fp->run();
        }
    } while (0);

    if (NULL != fp) {
        fp->finish();
        I_FREE(fp);
    }
    
    return;
}

void test_tcp_client() {
    Int32 ret = 0;
    Int32 fd = -1;
    int peer_port = 9999;
    Char peer_ip[32] = "172.8.1.12"; 
    FilePoll* fp = NULL;
    ProcHandler* hd = NULL;
    TcpBaseOper* msgoper = NULL;
    StdinOper* stdinoper = NULL;
    
    LOG_INFO("Enter server ip and server port:");
    scanf("%s %d", peer_ip, &peer_port);

    do {
        fd = creatTcpCli(peer_ip, peer_port);
        if (0 > fd) {
            break;
        }

        I_NEW(FilePoll, fp);
        ret = fp->init();
        if (0 != ret) {
            break;
        }

        I_NEW(ProcHandler, hd);
        
        I_NEW(TcpBaseOper, msgoper); 
        msgoper->setParam(hd);
        ret = fp->addOper(fd, msgoper, TRUE, TRUE, TRUE);
        if (0 != ret) {
            break;
        }
        
        I_NEW(StdinOper, stdinoper);
        stdinoper->setParam(fd, fp);
        ret = fp->addOper(0, stdinoper, TRUE, TRUE, FALSE);
        if (0 != ret) {
            break;
        }

        while (TRUE) {
            fp->run();
        }
    } while (0);

    if (NULL != fp) {
        fp->finish();
        I_FREE(fp);
    }
    
    return;
}

Int32 tcp_file_srv() {
    Int32 ret = 0;
    int port;
    Char ip[DEF_IP_SIZE];
    TcpFileSrv* srv = NULL;

    LOG_INFO("Enter server ip and port:");
    scanf("%s %d", ip, &port);

    I_NEW(TcpFileSrv, srv);
    srv->setParam(ip, port);

    do {
        ret = srv->init();
        if (0 != ret) {
            break;
        }

        srv->setConfSpeed(0x1000, 0x1000);

        ret = srv->start();
        if (0 != ret) {
            break;
        }
    } while (0);

    srv->finish();
    I_FREE(srv);
    
    return ret; 
}

Int32 tcp_file_cli() {
    Int32 ret = 0;
    Int32 opt = 0;
    int port;
    Char ip[DEF_IP_SIZE];
    TcpFileCli* cli = NULL;
    Void* msg = NULL;

    LOG_INFO("Enter connect server ip and port:");
    scanf("%s %d", ip, &port);

    LOG_INFO("Enter your option<0-upload, 1-download>:");
    scanf("%d", &opt);

    I_NEW(TcpFileCli, cli);
    cli->setParam(ip, port);

    do {
        ret = cli->init();
        if (0 != ret) {
            break;
        }

        cli->setConfSpeed(0x100, 0x100);

        if (0 == opt) {
            msg = test_upload();
            cli->uploadFile((EvMsgStartUpload*)msg);
        } else {
            msg = test_download();
            cli->downloadFile((EvMsgStartDownload*)msg);
        }

        ret = cli->start();
        if (0 != ret) {
            break;
        }
    } while (0);

    cli->finish();
    I_FREE(cli);
    
    return ret; 
}


int test_main(int opt) {
    int ret = 0;

    if (1 == opt || opt == 2) {
        ret = test_ctx();
    } else if (3 == opt) {
        test_node();
    } else if (4 == opt) {
        test_udp();
    } else if (5 == opt) {
        test_tcp_server();
    } else if (6 == opt) {
        test_tcp_client();
    } else if (7 == opt) {
        tcp_file_srv();
    } else if (8 == opt) {
        tcp_file_cli();
    } else {
    }

    //sysPause();
    return ret;
}
