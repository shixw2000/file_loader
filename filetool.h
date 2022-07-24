#ifndef __FILETOOL_H__
#define __FILETOOL_H__
/****
    * This file is used a file tool for common file operations.
****/
#include"globaltype.h"
#include"filectx.h"


enum FileOperFlag {
    FILE_FLAG_NULL = 0,
    FILE_FLAG_RD = 0x1,
    FILE_FLAG_WR = 0x2,
    FILE_FLAG_RD_WR,
};

struct iovec;

class FileTool {
public: 
    static Int32 statFile(const char path[], FileInfoType* info);

    static Int32 existsFile(const Char path[]);
  
    static Int32 openFile(const Char szPath[], Int32 type);
    
    static Int32 creatFile(const Char szPath[]);
    
    static Int32 delFile(const Char szPath[]); 

    static Int32 closeFd(Int32* pfd);

    /* return: 0: all ok,  1: busy, -1: end, -2: error */
    static Int32 preadFile(Int32 fd, Void* buf, Int32 size,
        Uint64 offset, Int32* out = NULL);

    /* return: 0: all ok,  1: busy,  -2: error */
    static Int32 pwriteFile(Int32 fd, const Void* buf, Int32 size,
        Uint64 offset, Int32* out = NULL);

    /* return: 0: all ok,  1: busy, -1: end, -2: error */
    static Int32 preadvFile(Int32 fd, 
        const struct iovec *iov, int iovcnt,
        Uint64 offset, Int32* out = NULL);

    /* return: 0: all ok,  1: busy,  -2: error */
    static Int32 pwritevFile(Int32 fd, 
        const struct iovec *iov, int iovcnt,
        Uint64 offset, Int32* out = NULL);

    static Int32 seekFile(int fd, Int64 offset);

    static Int32 calcFileBlkCnt(Int64 file_size);

    static Int32 calcBlkSize(Int32 beg, Int32 end, Int64 fileSize);

    static Bool chkBlkValid(Int32 beg, Int32 end, Int32 min, Int32 max);

    static Int32 getMapHeader(Int32 fd, FileMapHeader* oInfo);

    static Int32 creatMapHeader(Int32 fd, const FileMapHeader* oInfo);
    

private: 
    
    /* return: 0: all ok,  1: busy, -1: end, -2: error */
    static Int32 readFile(Int32 fd, Void* buf, Int32 size,
        Int32* out = NULL);

    /* return: 0: all ok,  1: busy,  -2: error */
    static Int32 writeFile(Int32 fd, const Void* buf, Int32 size,
        Int32* out = NULL);
};

#endif

