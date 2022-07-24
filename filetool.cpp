#define _FILE_OFFSET_BITS 64

#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/uio.h>
#include"filetool.h"


Int32 FileTool::closeFd(Int32* pfd) {
    if (NULL != pfd) {
        if (0 <= *pfd) {
            close(*pfd);

            *pfd = -1;
        }
    }

    return 0;
}

Int32 FileTool::statFile(const char path[], FileInfoType* info) {
    Int32 ret = 0;
    struct stat buf;
    
    ret = lstat(path, &buf);
    if (0 != ret) {
        return -1;
    }

    info->m_file_size = (Int64)buf.st_size;
    
    if (S_ISREG(buf.st_mode)) {
        info->m_file_type = ENUM_FILE_TYPE_REG;
    } else if (S_ISDIR(buf.st_mode)) {
        info->m_file_type = ENUM_FILE_TYPE_DIR;
    } else {
        info->m_file_type = ENUM_FILE_TYPE_OTHER;
    }

    return 0;
}



Int32 FileTool::getMapHeader(Int32 fd, FileMapHeader* hd) {
    Int32 ret = 0;
    Int32 blkCnt = 0;

    do { 
        ret = preadFile(fd, hd, sizeof(FileMapHeader), 0);
        if (0 != ret) {
            ret = -2;
            break;
        }

        if (DEF_MAGIC_NUM != hd->m_magic_flag) {
            ret = -3;
            break;
        }

        if (DEF_FILE_UNIT_SIZE != hd->m_file_unit) {
            ret = -4;
            break;
        } 

        blkCnt = calcFileBlkCnt(hd->m_file_size);
        if (blkCnt != hd->m_blk_cnt) {
            ret = -5;
            break;
        }

        if (0 > hd->m_blk_next || hd->m_blk_next > hd->m_blk_cnt) {
            ret = -6;
            break;
        }

        if (hd->m_completed && hd->m_blk_next != hd->m_blk_cnt) {
            ret = -7;
            break;
        }
        
        ret = 0;
    } while (0);

    return ret;
}

Int32 FileTool::creatMapHeader(Int32 fd, const FileMapHeader* hd) {
    Int32 ret = 0;
    Int32 size = sizeof(FileMapHeader);
   
    do {
        ret = ftruncate(fd, size);
        if (0 != ret) {
            ret = -1;
            break;
        }

        ret = pwriteFile(fd, hd, size, 0);
        if (0 != ret) {
            ret = -2;
            break;
        }

        ret = 0;
    } while (0);

    return ret;
}

/* return: 0: exists, 1: no-exists, other: fail */
Int32 FileTool::existsFile(const Char path[]) {
    Int32 ret = 0;
    struct stat buf;

    do {
        ret = lstat(path, &buf);
        if (0 != ret) {
            if (ENOENT == errno) {
                ret = 1;
            } else {
                ret = -2;
            }
            
            break;
        }

        if (!S_ISREG(buf.st_mode)) {
            ret = -3;
            break;
        }

        ret = 0;
    } while (0);

    return ret;
}


Int32 FileTool::openFile(const Char szPath[], Int32 type) {
    Int32 fd = -1;
    Int32 flag = 0;

    if (FILE_FLAG_RD == type) {
        flag = O_RDONLY;
    } else if (FILE_FLAG_WR == type) {
        flag = O_WRONLY;
    } else {
        flag = O_RDWR;
    }
    
    fd = open(szPath, flag);
    if (-1 != fd) {
        return fd;
    } else {
        return -1;
    }
}

Int32 FileTool::creatFile(const Char szPath[]) {
    Int32 fd = -1;
    Int32 flag = 0;
    Int32 mode = 0644; 
    
    flag = O_RDWR | O_EXCL | O_CREAT; 
    
    fd = open(szPath, flag, mode);
    if (-1 != fd) {
        return fd;
    } else {
        return -1;
    }
}

Int32 FileTool::delFile(const Char szPath[]) {
    Int32 ret = 0;
    
    ret = unlink(szPath);
    if (0 == ret) {
        return ret;
    } else {
        return -1;
    }
}

/* return: 0: all ok,  1: busy, -1: end, -2: error */
Int32 FileTool::readFile(Int32 fd, Void* buf, Int32 size, Int32* out) {
    Int32 ret = 0;
    Int32 rdlen = 0;
    Int32 total = 0;
    Char* psz = static_cast<Char*>(buf);

    while (0 < size) {
        rdlen = read(fd, psz, size);
        if (0 < rdlen) {
            total += rdlen;

            size -= rdlen;
            psz += rdlen;
        } else if (0 == rdlen) {
            /* end of file */
            ret = -1;
            break;
        } else if (EINTR == errno) {
            continue;
        } else if (EAGAIN == errno) {
            ret = 1;
            break;
        } else {
            ret = -2;
            break;
        }
    }

    if (NULL != out) {
        *out = total;
    }

    return ret;
}

/* return: 0: all ok,  1: busy, -1: end, -2: error */
Int32 FileTool::preadFile(Int32 fd, Void* buf, Int32 size,
    Uint64 offset, Int32* out) {
    Int32 ret = 0;
    Int32 total = 0;
    Int32 rdlen = 0;
    Char* psz = static_cast<Char*>(buf);

    while (0 < size) {
        rdlen = pread(fd, psz, size, offset);
        if (0 < rdlen) {
            total += rdlen;
            
            size -= rdlen;
            psz += rdlen;
            offset += rdlen;
        } else if (0 == rdlen) {
            /* end of file */
            ret = -1;
            break;
        } else if (EINTR == errno) {
            continue;
        } else if (EAGAIN == errno) {
            ret = 1;
            break;
        } else {
            ret = -2;
            break;
        }
    }

    if (NULL != out) {
        *out = total;
    }

    return ret;
}

/* return: 0: all ok,  1: busy,  -2: error */
Int32 FileTool::writeFile(Int32 fd, const Void* buf, Int32 size, Int32* out) {
    Int32 ret = 0;
    Int32 total = 0;
    Int32 wrlen = 0;
    const Char* psz = static_cast<const Char*>(buf);

    while (0 < size) {
        wrlen = write(fd, psz, size);
        if (0 < wrlen) {
            total += wrlen;
            
            size -= wrlen;
            psz += wrlen;
        } else if (0 == wrlen) {
            continue;
        } else if (EINTR == errno) {
            continue;
        } else if (EAGAIN == errno) {
            ret = 1;
            break;
        } else {
            ret = -2;
            break;
        }
    }

    if (NULL != out) {
        *out = total;
    }

    return ret;
}


/* return: 0: all ok,  1: busy,  -2: error */
Int32 FileTool::pwriteFile(Int32 fd, const Void* buf, Int32 size,
    Uint64 offset, Int32* out) {
    Int32 ret = 0;
    Int32 total = 0;
    Int32 wrlen = 0;
    const Char* psz = static_cast<const Char*>(buf);

    while (0 < size) {
        wrlen = pwrite(fd, psz, size, offset);
        if (0 < wrlen) {
            total += wrlen;
            
            size -= wrlen;
            psz += wrlen;
            offset += wrlen;
        } else if (0 == wrlen) {
            continue;
        } else if (EINTR == errno) {
            continue;
        }else if (EAGAIN == errno) {
            ret = 1;
            break;
        } else {
            ret = -2;
            break;
        }
    }

    if (NULL != out) {
        *out = total;
    }

    return ret;
}

/* return: 0: all ok,  1: busy, -1: end, -2: error */
Int32 FileTool::preadvFile(Int32 fd, 
    const struct iovec *iov, int iovcnt,
    Uint64 offset, Int32* out) {
    Int32 ret = 0;
    Int32 size = 0;
    Int32 total = 0;
    Int32 rdlen = 0;

    for (int i=0; i<iovcnt; ++i) {
        size += (Int32)iov[i].iov_len;
    }
    
    while (0 < size) {
        rdlen = preadv(fd, iov, iovcnt, offset);
        if (0 < rdlen) {
            total += rdlen;
            
            if (total == size) {
                ret = 0;
            } else {
                ret = 1;
            }

            break;
        } else if (0 == rdlen) {
            /* end of file */
            ret = -1;
            break;
        } else if (EINTR == errno) {
            continue;
        } else if (EAGAIN == errno) {
            ret = 1;
            break;
        } else {
            ret = -2;
            break;
        }
    }

    if (NULL != out) {
        *out = total;
    }

    return ret;
}

/* return: 0: all ok,  1: busy,  -2: error */
Int32 FileTool::pwritevFile(Int32 fd, 
    const struct iovec *iov, int iovcnt,
    Uint64 offset, Int32* out) {
    Int32 ret = 0;
    Int32 size = 0;
    Int32 total = 0;
    Int32 wrlen = 0;

    for (int i=0; i<iovcnt; ++i) {
        size += (Int32)iov[i].iov_len;
    }

    while (0 < size) {
        wrlen = pwritev(fd, iov, iovcnt, offset);
        if (0 < wrlen) {
            total += wrlen;
            
            if (total == size) {
                ret = 0;
            } else {
                ret = 1;
            }

            break;
        } else if (0 == wrlen) {
            continue;
        } else if (EINTR == errno) {
            continue;
        }else if (EAGAIN == errno) {
            ret = 1;
            break;
        } else {
            ret = -2;
            break;
        }
    }

    if (NULL != out) {
        *out = total;
    }

    return ret;
}


Int32 FileTool::seekFile(int fd, Int64 offset) {
    off_t newPos = 0;
    off_t oldPos = (off_t)offset;

    newPos = lseek(fd, oldPos, SEEK_SET);
    if (newPos == oldPos) {
        return 0;
    } else {
        return -1;
    }
}

Int32 FileTool::calcFileBlkCnt(Int64 file_size) {

    file_size += DEF_FILE_UNIT_MASK;
    file_size >>= DEF_FILE_UNIT_SHIFT_BITS;
    
    return (Int32)file_size;
}

/** [beg, end) **/
Int32 FileTool::calcBlkSize(Int32 beg, Int32 end, Int64 fileSize) {
    Int32 size = 0;
    Int32 max = calcFileBlkCnt(fileSize);

    if (0 <= beg && beg < end && end <= max) {
        if (end < max) {
            size = (end - beg) << DEF_FILE_UNIT_SHIFT_BITS;
        } else {
            if (fileSize & DEF_FILE_UNIT_MASK) {
                size = ((max - beg - 1) << DEF_FILE_UNIT_SHIFT_BITS)
                    + (Int32)(fileSize & DEF_FILE_UNIT_MASK);
            } else {
                size = (end - beg) << DEF_FILE_UNIT_SHIFT_BITS;
            }
        }

        return size;
    } else {
        return 0;
    }
}

Bool FileTool::chkBlkValid(Int32 beg, Int32 end, Int32 min, Int32 max) {        
    if (min <= beg && beg <= end && end <= max
        && (end - beg) <= DEF_MAX_FRAME_SIZE) {
        return TRUE;
    } else {
        return FALSE;
    }
}

