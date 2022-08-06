#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/timerfd.h>
#include<sys/eventfd.h>
#include"errno.h"
#include"sockutil.h"


Int32 creatUdpSock(const Char local_ip[], int local_port) {
    Int32 ret = 0;
    Int32 fd = -1;
    Int32 addr_len = 0;
    struct sockaddr_storage addr;
    
    addr_len = sizeof(struct sockaddr_storage);
    ret = creatAddr(local_ip, local_port, &addr, &addr_len);
    if (0 != ret) {
        return -1;
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (0 > fd) {
        LOG_INFO("create_udp| ret=%d| error=%s|",
            fd, ERR_MSG());

        return -1;
    }

    setNonBlock(fd); 
    
    ret = bind(fd, (struct sockaddr*)&addr, addr_len);
    if (0 == ret) {
        LOG_INFO("bind_udp| local_ip=%s| local_port=%d| msg=ok|",
            local_ip, local_port);
        
        return fd;
    } else {
        LOG_INFO("bind_udp| ret=%d| local_ip=%s| local_port=%d| error=%s|",
            ret, local_ip, local_port, ERR_MSG());
        
        closeHd(fd);
        return -1; 
    } 
}

Int32 creatTcpSrv(const Char local_ip[], int local_port) {
    Int32 ret = 0;
    Int32 fd = -1;
    Int32 addr_len = 0;
    struct sockaddr_storage addr;
    
    addr_len = sizeof(struct sockaddr_storage);
    ret = creatAddr(local_ip, local_port, &addr, &addr_len);
    if (0 != ret) {
        return -1;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 > fd) {
        LOG_INFO("create_tcp| ret=%d| error=%s|",
            fd, ERR_MSG());

        return -1;
    }

    do {
        setNonBlock(fd); 
        setReuse(fd);
        
        ret = bind(fd, (struct sockaddr*)&addr, addr_len);
        if (0 != ret) {
            LOG_INFO("bind_tcp| ret=%d| local_ip=%s| local_port=%d| error=%s|",
                ret, local_ip, local_port, ERR_MSG());
            
            break; 
        }

        ret = listen(fd, 1000);
        if (0 != ret) {
            LOG_INFO("listen_tcp| ret=%d| local_ip=%s| local_port=%d| error=%s|",
                ret, local_ip, local_port, ERR_MSG());
            
            break; 
        } 

        LOG_INFO("listen_tcp| local_ip=%s| local_port=%d| msg=ok|",
            local_ip, local_port);
        
        return fd;
    } while (0);

    closeHd(fd);
    return -1;
}

Int32 creatTcpCli(const Char peer_ip[], int peer_port) {
    Int32 ret = 0;
    Int32 fd = -1;
    Int32 addr_len = 0;
    struct sockaddr_storage addr;
    
    addr_len = sizeof(struct sockaddr_storage);
    ret = creatAddr(peer_ip, peer_port, &addr, &addr_len);
    if (0 != ret) {
        return -1;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 > fd) {
        LOG_INFO("create_tcp| ret=%d| error=%s|",
            fd, ERR_MSG());

        return -1;
    }

    ret = connAddr(fd, &addr, addr_len);
    if (0 == ret) {
        setNonBlock(fd); 
        
        LOG_INFO("connect_tcp| peer_ip=%s| peer_port=%d| msg=ok|",
            peer_ip, peer_port);

        return fd;
    } else {
        LOG_INFO("connect_tcp| ret=%d| peer_ip=%s| peer_port=%d| error=%s|",
            ret, peer_ip, peer_port, ERR_MSG());
        
        closeHd(fd);
        return -1;
    } 
}

Int32 setNonBlock(Int32 fd) {
    Int32 ret = 0;

    ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    return ret;
}

Int32 setReuse(Int32 fd) {
    Int32 ret = 0;
    Int32 val = 1;
	Int32 len = sizeof(val);
    
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, len);
    if (0 != ret) {
        LOG_INFO("set_reuse| fd=%d| ret=%d| error=%s|", 
            fd, ret, ERR_MSG());
    }

    return ret;
}

Int32 creatAddr(const Char ip[], int port, Void* addr, Int32* plen) {
    Int32 ret = 0;
    struct sockaddr_in* dst = (struct sockaddr_in*)addr;

    memset(addr, 0, *plen);
    dst->sin_family = AF_INET;
    dst->sin_port = htons(port);
    ret = inet_pton(AF_INET, ip, &dst->sin_addr);
    if (1 == ret) {
        *plen = sizeof(struct sockaddr_in);
        return 0;
    } else {
        *plen = 0;
        return -1;
    }
}

Int32 parseAddr(const Void* addr, Char ip[], int* port) {
    const char* psz = NULL;
    const struct sockaddr_in* src = (const struct sockaddr_in*)addr;
    
    psz = inet_ntop(AF_INET, &src->sin_addr, ip, DEF_IP_SIZE);
    if (NULL != psz) {
        *port = ntohs(src->sin_port);
        return 0;
    } else {
        *port = 0;
        return -1;
    }
}

Int32 getLocalInfo(Int32 fd, Char ip[], int* port) {
    Int32 ret = 0;
    socklen_t addr_len = 0;
    struct sockaddr_storage addr;

    addr_len = sizeof(addr);
    ret = getsockname(fd, (struct sockaddr*)&addr, &addr_len);
    if (0 != ret) {
        return -1;
    }

    ret = parseAddr(&addr, ip, port);
    if (0 != ret) {
        return -1;
    }

    return 0;
}

Int32 getPeerInfo(Int32 fd, Char ip[], int* port) {
    Int32 ret = 0;
    socklen_t addr_len = 0;
    struct sockaddr_storage addr;

    addr_len = sizeof(addr);
    ret = getpeername(fd, (struct sockaddr*)&addr, &addr_len);
    if (0 != ret) {
        return -1;
    }

    ret = parseAddr(&addr, ip, port);
    if (0 != ret) {
        return -1;
    }

    return 0;
}

Int32 closeHd(Int32 fd) {
    Int32 ret = 0;
    
    if (0 <= fd) {
        ret = close(fd);
    }

    return ret;
}

Int32 connAddr(Int32 fd, const Void* addr, Int32 len) {
    Int32 ret = 0;

    ret = connect(fd, (struct sockaddr*)addr, len);
    if (0 == ret) {
        return 0;
    } else {
        LOG_INFO("connect| fd=%d| ret=%d| error=%s|",
            fd, ret, ERR_MSG());
        return -1;
    }
}

Int32 sendUdp(Int32 fd, const Void* buf, Int32 len, 
    const Void* addr, Int32 addr_len) {
    Int32 sndlen = 0;

    if (0 < len) {
        sndlen = sendto(fd, buf, len, 0, (struct sockaddr*)addr, addr_len);
        if (0 <= sndlen) {
            LOG_INFO("sendUdp| fd=%d| len=%d| sndlen=%d| msg=ok|",
                fd, len, sndlen);
            
            return sndlen; 
        } else if (EAGAIN == errno || EINTR == errno) {
            return 0;
        } else if (EMSGSIZE == errno || ENOMEM == errno) {
            LOG_INFO("sendUdp| fd=%d| len=%d| error=drop:%s|",
                fd, len, ERR_MSG());
            
            return -2;
        } else {
            LOG_INFO("sendUdp| fd=%d| len=%d| error=%s|",
                fd, len, ERR_MSG());
            return -1;
        }
    } else {
        return 0;
    }
}

Int32 recvUdp(Int32 fd, Void* buf, Int32 maxlen,
    Void* addr, int* addr_len) {
    Int32 rdlen = 0;
    socklen_t slen = 0;

    if (0 < maxlen) {
        if (NULL != addr_len) {
            slen = *addr_len;
        }
        
        /* udp msg may be truncated by size */
        rdlen = recvfrom(fd, buf, maxlen, 0, (struct sockaddr*)addr, &slen);
        if (0 <= rdlen) {
            LOG_INFO("recvUdp| fd=%d| maxlen=%d| rdlen=%d|"
                " addr_len=%d| msg=ok|",
                fd, maxlen, rdlen, slen);

            if (NULL != addr_len) {
                *addr_len = slen;
            }
            
            return rdlen;
        } else if (EAGAIN == errno || EINTR == errno) {
            return 0;
        } else {
            LOG_INFO("recvUdp| fd=%d| maxlen=%d| error=%s|",
                fd, maxlen, ERR_MSG());
            
            return -1;
        }
    } else {
        return 0;
    }
}

Int32 sendTcp(Int32 fd, const Void* buf, Int32 len) {
    Int32 sndlen = 0;

    if (0 < len) {
        sndlen = send(fd, buf, len, MSG_NOSIGNAL);
        if (0 <= sndlen) {
            LOG_DEBUG("sendTcp| fd=%d| len=%d| sndlen=%d|"
                " msg=ok|",
                fd, len, sndlen);
            
            return sndlen;
        } else if (EAGAIN == errno || EINTR == errno) {
            return 0;
        }  else {
            LOG_INFO("sendTcp| fd=%d| len=%d| ret=%d| error=%s|",
                fd, len, sndlen, ERR_MSG());
            return -1;
        }
    } else {
        return 0;
    }
}

Int32 recvTcp(Int32 fd, Void* buf, Int32 maxlen) {
    Int32 rdlen = 0;

    if (0 < maxlen) { 
        rdlen = recv(fd, buf, maxlen, 0);
        if (0 < rdlen) {
            LOG_DEBUG("recvTcp| fd=%d| maxlen=%d| rdlen=%d| msg=ok|",
                fd, maxlen, rdlen);
            
            return rdlen;
        } else if (0 == rdlen) {
            /* end of read */
            LOG_INFO("recvTcp| fd=%d| maxlen=%d| error=eof|",
                fd, maxlen);
            return -2;
        } else if (EAGAIN == errno || EINTR == errno) {
            return 0;
        } else {
            LOG_INFO("recvTcp| fd=%d| maxlen=%d| error=%s|",
                fd, maxlen, ERR_MSG());
            
            return -1;
        }
    } else {
        return 0;
    }
}

/* return >=0: ok fd, -1: error, -2: empty */
Int32 acceptCli(Int32 fd) {
    Int32 newFd = -1;

    newFd = accept(fd, NULL, NULL);
    if (0 <= newFd) {
        setNonBlock(newFd);
        return newFd;
    } else if (EAGAIN == errno || EINTR == errno) {
        return -2;
    } else {
        return -1;
    }
}

Int32 peekTcp(Int32 fd, Void* buf, Int32 maxlen) {
    Int32 rdlen = 0;

    rdlen = recv(fd, buf, maxlen, MSG_PEEK);
    if (0 < rdlen) {
        LOG_INFO("peekTcp| fd=%d| maxlen=%d| rdlen=%d| msg=ok|",
            fd, maxlen, rdlen);
        
        return rdlen;
    } else if (0 == rdlen) {
        /* end of read */
        LOG_INFO("peekTcp| fd=%d| maxlen=%d| error=eof|",
            fd, maxlen);
        return -2;
    } else if (EAGAIN == errno || EINTR == errno) {
        return 0;
    } else {
        LOG_INFO("peekTcp| fd=%d| maxlen=%d| error=%s|",
            fd, maxlen, ERR_MSG());
        
        return -1;
    }
}


Int32 writeEvent(int fd) {
    Int32 ret = 0;
    Int32 len = sizeof(Uint64);
    Uint64 val = 1;

    ret = write(fd, &val, len);
    if (ret == len) {
        return 0;
    } else {
        return -1;
    }
}

Int32 readEvent(int fd, Uint32* pVal) {
    Int32 ret = 0;
    Int32 len = sizeof(Uint64);
    Uint64 val = 1;

    ret = read(fd, &val, len);
    if (ret == len) {
        if (NULL != pVal) {
            *pVal = (Uint32)val;
        }
        
        return 0;
    } else {
        if (NULL != pVal) {
            *pVal = 0;
        }
        
        return -1;
    }
}

Int32 creatTimerFd(Int32 ms) {
    Int32 ret = 0;
    Int32 fd = -1;
    struct itimerspec value;

    fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (0 > fd) {
        return -1;
    }

    value.it_value.tv_sec = 0;
	value.it_value.tv_nsec = 1;
    value.it_interval.tv_sec = 0;
	value.it_interval.tv_nsec = ms * 1000000;
    ret = timerfd_settime(fd, 0, &value, NULL);
    if (0 != ret) {
        close(fd);
        return -1;
    }

    return fd;
}

Int32 creatEventFd() {
    Int32 fd = -1; 

    fd = eventfd(1, EFD_NONBLOCK);
    if (0 > fd) {
        return -1;
    } 
    
    return fd;
}

Void sysPause() {
    pause();
}
