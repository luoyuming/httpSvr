/*
*  12797202@qq.com   name: luoyuming mobile 13925236752
*
* Copyright (c) 2020 www.dswd.net
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
* LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
* OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
* WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "asynFile.h"
#include "handleMsg.h"


CAsynFile::CAsynFile()
{
#ifdef WIN_OS 
    m_convert = std::make_shared<CConvertSet>();
#endif
    m_compress = std::make_shared<CompressMng>();

    m_contentType = std::make_shared<ContentTypeMng>(false);
}

CAsynFile::~CAsynFile()
{

}

void CAsynFile::handleAsynFile(std::shared_ptr<PACKAGE_INFO> & pPkgInfo)
{
    auto asynPtr = std::make_shared<ASYN_FILE_INFO>();
    ASYN_FILE_INFO & asynInfo = *asynPtr;
    asynInfo.pTimer = std::make_shared<UTIL_SELF::Timer>();
    asynInfo.pos = 0;
    asynInfo.pPkgInfo = pPkgInfo;
#ifdef WIN_OS   
    DWORD  dwError = 0;
    wstring fileName;
    m_convert->UTF8ToUnicode(pPkgInfo->fileNameAsyn, fileName);   
    HANDLE hFile = CreateFile(fileName.c_str(),               // file to open
        GENERIC_READ,          // open for reading
        FILE_SHARE_READ,       // share for reading
        NULL,                  // default security
        OPEN_EXISTING,         // existing file only
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // normal file      
        NULL);
    if (INVALID_HANDLE_VALUE == hFile) {
        dwError = GetLastError();
        LOG_ERROR("fault to open flie (error code %d)(%s)", dwError, pPkgInfo->fileNameAsyn.c_str());
        return;
    }
    OVERLAPPED ol = { 0 };
    
    asynInfo.overlapped = ol;
    asynInfo.overlapped.Offset = 0;    
    asynInfo.hFile = hFile;
    
    int  & fileSize = asynInfo.fileSize;
    if (!GetFileSizeEx(hFile,(PLARGE_INTEGER)&fileSize)) {
        dwError = GetLastError();
        LOG_ERROR("fault to read flie size (error code %d)(%s)", dwError, pPkgInfo->fileNameAsyn.c_str());
        return;
    }    
    asynInfo.pPkgInfo->strResp.resize(fileSize);
    DWORD dwBytesRead = 0;
    BOOL bResult = ReadFile(asynInfo.hFile, asynInfo.buffer, DATA_BUFSIZE, &dwBytesRead, &(asynInfo.overlapped));
    if (bResult) {
        string & strRef = asynInfo.pPkgInfo->strResp;
        memcpy(&strRef[asynInfo.pos], asynInfo.buffer, dwBytesRead);
        asynInfo.overlapped.Offset += dwBytesRead;
        asynInfo.pos = asynInfo.overlapped.Offset;
    }

    m_pkgInfo.push_back(asynPtr);
    if (bResult) {
        checkAsynReadStatus();
    }
#else  
 
    asynInfo.hFile = open(pPkgInfo->fileNameAsyn.c_str(), O_RDONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (asynInfo.hFile < 0) {      
        LOG_ERROR("fault to open flie (error code %d)(%s)(%s)", errno, strerror(errno), pPkgInfo->fileNameAsyn.c_str());
        return;
    }
    struct stat buf;
    if (fstat(asynInfo.hFile, &buf) < 0) {
        LOG_ERROR("fstat (error code %d)(%s)(%s)", errno, strerror(errno), pPkgInfo->fileNameAsyn.c_str());
        return;
    }

    asynPtr->fileSize = static_cast<int>(buf.st_size);  
    asynInfo.pPkgInfo->strResp.resize(asynPtr->fileSize);
    bzero(&(asynInfo.rd), sizeof(asynInfo.rd));
    auto & rd = asynInfo.rd;
    rd.aio_buf = asynInfo.buffer;
    rd.aio_fildes = asynInfo.hFile;
    rd.aio_nbytes = DATA_BUFSIZE;
    rd.aio_offset = 0;

    int ret = aio_read(&rd);
    if (ret < 0) {
        LOG_ERROR("aio_read (error code %d)(%s)(%s)", errno, strerror(errno), pPkgInfo->fileNameAsyn.c_str());
    }
    m_pkgInfo.push_back(asynPtr);
#endif
    return;
}

void CAsynFile::asynReadFile()
{  
    bool bResult = false;
    for (auto & asynInfo : m_pkgInfo) {
#ifdef WIN_OS 
        bResult = winAsynRead(asynInfo);
#else
        auto & rd = asynInfo->rd;
        if (aio_error(&rd) != EINPROGRESS) {
            int ret = aio_return(&rd);
            if (ret > 0) {
                string & strRef = asynInfo->pPkgInfo->strResp;
                memcpy(&strRef[asynInfo->pos], asynInfo->buffer, ret);
                asynInfo->pos += ret;

                auto & rd = asynInfo->rd;
                rd.aio_buf = asynInfo->buffer;
                rd.aio_fildes = asynInfo->hFile;
                rd.aio_nbytes = DATA_BUFSIZE;
                rd.aio_offset = asynInfo->pos;
                ret = aio_read(&rd);
                if (ret < 0) {
                    LOG_ERROR("aio_read (error code %d)(%s)", errno, strerror(errno));
                }
            }
        }

#endif
        if (bResult) {
            asynInfo->pTimer->reset();
        }
    }

    return;
}

#ifdef WIN_OS 
bool CAsynFile::winAsynRead(std::shared_ptr<ASYN_FILE_INFO> & asynInfo)
{   
    bool bRet = false;
    DWORD dwBytesRead = 0;
    BOOL bResult = GetOverlappedResult(asynInfo->hFile, &(asynInfo->overlapped), &dwBytesRead, INFINITE);
    if (bResult) {
        bRet = true;
        string & strRef = asynInfo->pPkgInfo->strResp;
        memcpy(&strRef[asynInfo->pos], asynInfo->buffer, dwBytesRead);
        asynInfo->overlapped.Offset += dwBytesRead;
        asynInfo->pos = asynInfo->overlapped.Offset;

        bResult = ReadFile(asynInfo->hFile, asynInfo->buffer, DATA_BUFSIZE, &dwBytesRead, &(asynInfo->overlapped));
        if (bResult) {
            string & strRef = asynInfo->pPkgInfo->strResp;
            memcpy(&strRef[asynInfo->pos], asynInfo->buffer, dwBytesRead);
            asynInfo->overlapped.Offset += dwBytesRead;
            asynInfo->pos = asynInfo->overlapped.Offset;
        }
    }

    return bRet;
}
#endif


void CAsynFile::doSuccess(std::shared_ptr<ASYN_FILE_INFO> & asynInfo)
{
    string extName = UTIL_SELF::getExtName(asynInfo->pPkgInfo->fileNameAsyn);   
    if (m_contentType->findExtName(extName)) {
        m_compress->encode(asynInfo->pPkgInfo);
    }  

#ifdef HTTP_V2
    if (PORT_TYPE::SOCKET_HTTPS == asynInfo->pPkgInfo->channel->portType)
          asynInfo->pPkgInfo->bH2RespHead = true;
#endif
    asynInfo->pPkgInfo->fileNameAsyn.clear();
    CHandleMsgS->handleResp(asynInfo->pPkgInfo);
    return;
}

void CAsynFile::checkAsynReadStatus()
{
    auto it = m_pkgInfo.begin();
    for (; it != m_pkgInfo.end(); ) {
        auto asynInfo = *it;
        if (asynInfo->pos >= asynInfo->fileSize) {
            doSuccess(asynInfo);

            CloseHandle(asynInfo->hFile);

            it = m_pkgInfo.erase(it);
        }
        else {
            ++it;
        }
    }
    return;
}

int CAsynFile::getWaitTime()
{
    int ret = MAX_ASYN_WAIT_MS;
    if (m_pkgInfo.size() > 0) {
        ret = 1;
        handleTimeOver();
    }    
    return ret;
}

void CAsynFile::handleTimeOver()
{
    auto it = m_pkgInfo.begin();
    for (; it != m_pkgInfo.end(); ) {
        auto asynInfo = *it;
        if ((*it)->pTimer->elapsed_seconds() >= MAX_IDLE_SEC) {
            string & file = asynInfo->pPkgInfo->fileNameAsyn;
            LOG_ERROR("time over file(%s)", file.c_str());

            CloseHandle((*it)->hFile);
            it = m_pkgInfo.erase(it);
        }
        else {
            ++it;
        }
    }
    return;
}