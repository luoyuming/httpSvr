#pragma once
#include "common.h"
#include "util.h"
#include "log.h"
#include "socketService.h"
#include "taskQueue.h"
#include "convertSet.h"
#include "compressMng.h"
#include "contentTypeMng.h"

struct ASYN_FILE_INFO {
    std::shared_ptr<PACKAGE_INFO> pPkgInfo;
    HANDLE hFile;
    int fileSize;
    int pos;
    char buffer[DATA_BUFSIZE];  
    std::shared_ptr<UTIL_SELF::Timer> pTimer;
#ifdef WIN_OS 
    OVERLAPPED overlapped;
#else
    struct aiocb   rd;
#endif
};

class CAsynFile
{  
public:
    CAsynFile();
    ~CAsynFile();
    void handleAsynFile(std::shared_ptr<PACKAGE_INFO> & pPkgInfo);
    void asynReadFile();
    void checkAsynReadStatus();
    int getWaitTime();
private:
    void handleTimeOver();
    void doSuccess(std::shared_ptr<ASYN_FILE_INFO> & asynInfo);
#ifdef WIN_OS 
    bool winAsynRead(std::shared_ptr<ASYN_FILE_INFO> & asynInfo);  
    
    std::shared_ptr<CConvertSet>                    m_convert;
#else
    
#endif
    std::shared_ptr<ContentTypeMng>                 m_contentType;
    std::list<std::shared_ptr<ASYN_FILE_INFO>>      m_pkgInfo;
    std::shared_ptr<CompressMng>                    m_compress;
};
