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


#include "common.h"
#include "socketService.h"
#include "log.h"
#include "handleMsg.h"
#include "configMng.h"
#include "h2Frame.h"
#include "h2Mng.h"



CSocketService::CSocketService()
{
    g_sslTemp = nullptr;

    m_listenSocketTcp = 0;
    m_listenSocketHttp = 0;
    m_listenSocketSSL = 0;
    m_handlePort = HANDLE_NULL;
#ifdef WIN_OS 
    m_acceptEx = nullptr;  
#endif

    m_mpMothed[TRACE_MOTHED] = HTTP_TRACE;
    m_mpMothed[OPTIONS_MOTHED] = HTTP_OPTIONS;
    m_mpMothed[CONNECT_MOTHED] = HTTP_CONNECT;
    m_mpMothed[DELETE_MOTHED] = HTTP_DELETE;
    m_mpMothed[PUT_MOTHED] = HTTP_PUT;
    m_mpMothed[HEAD_MOTHED] = HTTP_HEAD;
    m_mpMothed[GET_MOTHED] = HTTP_GET;
    m_mpMothed[POST_MOTHED] =  HTTP_POST;

    m_liLN.push_back(HEAD_END_FLAG);
    m_liLN.push_back(HEAD_END_FLAG_ONE);
    m_liLN.push_back(HEAD_END_FLAG_TWO);
}

CSocketService::~CSocketService()
{    
#ifdef WIN_OS 
    WSACleanup();
#endif

    for (auto & ctx : m_mpCtxSSL) {
        SSL_CTX_free((SSL_CTX*)ctx.second);
    }
    ERR_free_strings();
}

void CSocketService::stop()
{
#ifdef WIN_OS 
    if (!m_handlePort) {
        return;
    }    
    PostQueuedCompletionStatus(m_handlePort, 0, (ULONG_PTR)IOType::IOClose, 0);
    CloseHandle(m_handlePort);
    m_handlePort = HANDLE_NULL;
#endif

}

bool CSocketService::init(XML_SYSTEM_CONFIG && config)
{   

#ifdef WIN_OS 
    WSADATA wa;
    if (0 != WSAStartup(MAKEWORD(2, 2), &wa)) {

        LOG_ERROR("fault to call WSAStartup");
        return false;
    }
    m_handlePort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0);
    if (!m_handlePort) {
        LOG_ERROR("fault to call CreateIoCompletionPort");
        return false;
    }
#else 
    m_handlePort = epoll_create(1000);
    if (-1 == m_handlePort)
    {
        LOG_ERROR("error to create epoll!");
        return false;
    }

#endif

    m_ipType = (IP_V4_TYPE == config.ip_v)? IP_TYPE ::IP_V4: IP_TYPE::IP_V6;  

    int tcpPort = atoi(config.tcp_port.c_str());
    int httpPort = atoi(config.http_port.c_str());
    int httpsPort = atoi(config.https_port.c_str());
   
    if (tcpPort > 0) {
        if (!createSocket(m_listenSocketTcp, tcpPort)) {           
            return false;
        }
        LOG_INFO("successful to build tcp port(%d)", tcpPort);
    }
    if (httpPort > 0) {
        if (!createSocket(m_listenSocketHttp, httpPort)) {
            return false;
        }
        LOG_INFO("successful to build http port(%d)", httpPort);
    }
    if (httpsPort > 0) {
        if (!initSSL()) {
            LOG_ERROR("fault to initSSL");
            return false;
        }
        if (!createSocket(m_listenSocketSSL, httpsPort)) {
            return false;
        }
        LOG_INFO("successful to build ssl port(%d)", httpsPort);
    }  

    return true;
}


bool CSocketService::createSocket(SOCKET & sock, int port)
{
    int af = AF_INET;
    if (IP_TYPE::IP_V6 == m_ipType)
        af = AF_INET6;     
    sock = socket(af, SOCK_STREAM, IPPROTO_TCP);
    assert(sock != INVALID_SOCKET);

    struct sockaddr_in6 addr_in6 = { 0 };
    addr_in6.sin6_family = AF_INET6;
    addr_in6.sin6_port = htons(port);
    addr_in6.sin6_addr = in6addr_any;

    sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
   
    int reuseaddr = 1;
    int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseaddr, sizeof(reuseaddr));
    if (0 != res) {
        LOG_ERROR("fault to call setsockopt SO_REUSEADDR (%d)", port);
        closesocket(sock);
        return false;
    }
    int optint = 1;
    res = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&optint, sizeof(optint));
    if (0 != res) {
        LOG_ERROR("fault to call setsockopt TCP_NODELAY (%d)", port);
        closesocket(sock);
        return false;
    }

    
    setNonblocking(sock);
    initWSAfunc(sock);

    int nameLen = sizeof(sockaddr);
    struct sockaddr * addrInfo = (struct sockaddr *)&addr;
    if (IP_TYPE::IP_V6 == m_ipType) {
        addrInfo = (struct sockaddr *)&addr_in6;
        nameLen = sizeof(addr_in6);
    }
    if (0 != ::bind(sock, addrInfo, nameLen)) {

        LOG_ERROR("fault to call bind port(%d)", port);
        closesocket(sock);
        return false;
    }

    if (0 != listen(sock, 10)) {
        LOG_ERROR("fault to call listen port(%d)", port);
        closesocket(sock);
        return false;
    }

#ifdef WIN_OS 
    postAccept(sock);
    if (!notify(sock, IOType::IOAccept)) {
        LOG_ERROR("fault to notify(%d)", sock);
    }
#else
    struct epoll_event ev;
    ev.data.fd = sock;
    ev.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLRDHUP;
    int ret = epoll_ctl(m_handlePort, EPOLL_CTL_ADD, sock, &ev);
    if (0 != ret) {
        string strError = strerror(errno);
        LOG_ERROR("epoll_ctl add sock fail %d: %s", errno, strError.c_str());
        return false;
    }
#endif
    LOG_INFO("successful to build server port %d", port);
    return true;
}

bool CSocketService::notify(SOCKET sock, IOType type)
{
    
#ifdef WIN_OS 
    if (!CreateIoCompletionPort((HANDLE)sock, m_handlePort, (ULONG_PTR)type, 0)) {          
        LOG_ERROR("fault to call CreateIoCompletionPort error_no(%d)", getErrorNo());
        return false;
    }
#endif
   
    return true;
}


int  CSocketService::getErrorNo()
{
    int ret = 0;
#ifdef WIN_OS 
    ret = GetLastError();
#else
    ret = errno;
#endif
    return ret;
}

int CSocketService::setNonblocking(SOCKET sock)
{
#ifdef WIN_OS 
    u_long mode = 1;
    if (SOCKET_ERROR == ioctlsocket(sock, (long)FIONBIO, &mode)) {
        LOG_WARNING("fault to SetNonblocking %d", sock);
        return -1;
    }
#else 
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0)
    {
        LOG_WARNING("fault to SetNonblocking %d", sock);
        return -1;
    }

    opts |= O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0)
    {
        LOG_WARNING("fault to SetNonblocking %d", sock);
        return -1;
    }
#endif
    return 0;
}

bool CSocketService::initWSAfunc(SOCKET sock)
{
#ifdef WIN_OS 
    GUID guidAcceptEx = WSAID_ACCEPTEX;
    DWORD bytes = 0;
    if (WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx,
        sizeof(GUID), &m_acceptEx, sizeof(LPFN_ACCEPTEX), &bytes, NULL,
        NULL) == SOCKET_ERROR) {        
        LOG_ERROR("fault to call WSAIoctl WSAID_ACCEPTEX(%d)", getErrorNo());
        return false;
    }
                                
    GUID GetAcceptExaddrsGuid = WSAID_GETACCEPTEXSOCKADDRS;
    WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GetAcceptExaddrsGuid,
        sizeof(GetAcceptExaddrsGuid),  &m_getAcceptExSockaddrs,  sizeof(m_getAcceptExSockaddrs),
        &bytes, NULL, NULL);
#endif

    return true;
}

#ifdef WIN_OS 

void CSocketService::postAccept(SOCKET sock)
{
    DWORD len = 0;  
    auto session = std::make_shared<SESSION_INFO>();  

    auto recvIO = std::make_shared<PER_IO_DATA>();   
    recvIO->DataBuf.buf = recvIO->buffer;
    recvIO->DataBuf.len = DATA_BUFSIZE;
    recvIO->session = session;
    recvIO->type = IOType::IOAccept;

    auto sndIO = std::make_shared<PER_IO_DATA>();
    sndIO->DataBuf.buf = sndIO->buffer;
    sndIO->DataBuf.len = DATA_BUFSIZE;
    sndIO->session = session;    
    sndIO->type = IOType::IOAccept;

    int addrLen = sizeof(sockaddr_in)*2;
    int af = AF_INET;
    if (IP_TYPE::IP_V6 == m_ipType) {
        af = AF_INET6;
        addrLen = sizeof(sockaddr_in6)*2;
    }
    session->sockAccept = ::socket(af, SOCK_STREAM, IPPROTO_TCP);
    assert(session->sockAccept != INVALID_SOCKET);

    session->sockListen = sock;
    session->recvIO = recvIO;
    session->sndIO = sndIO;
    setNonblocking(session->sockAccept);

    
    if (!m_acceptEx(sock, session->sockAccept, recvIO->buffer, 0,
        addrLen, addrLen, &len,  &(recvIO->Overlapped))) {
        if (WSA_IO_PENDING != WSAGetLastError()) {
           
            LOG_WARNING("not WSA_IO_PENDING error(%d)", getErrorNo());
        }
    }
    {
        std::lock_guard<std::mutex>lck(m_lock);
        m_liSession.push_back(session);
    }

}

void CSocketService::releaseSession(std::shared_ptr<SESSION_INFO> & key)
{
    if (key) {
        if (key.get()) {
            std::lock_guard<std::mutex>lck(m_lock);
            auto iter = std::find(m_liSession.begin(), m_liSession.end(), key);
            if (iter != m_liSession.end()) {
                m_liSession.remove(key);
            }
        }
    }
}

void CSocketService::releaseIO(LPPER_IO_DATA perIoData)
{
    auto key = perIoData->session;
    {
        std::lock_guard<std::mutex>lck(m_lock);
        auto iter = std::find(m_liSession.begin(), m_liSession.end(), key);
        if (iter != m_liSession.end()) {
            m_liSession.remove(key);
        }
    }
    return;
}



void CSocketService::doAccept(LPPER_IO_DATA perIoData, int len)
{
    SOCKET listenFd = perIoData->session->sockListen;
    PORT_TYPE portType = PORT_TYPE::SOCKET_HTTP;
    if (m_listenSocketTcp == listenFd) {
        portType = PORT_TYPE::SOCKET_TCP;
    }
    else if (m_listenSocketSSL == listenFd) {
        portType = PORT_TYPE::SOCKET_HTTPS;
    }

    int addrLen = sizeof(sockaddr_in) * 2;    
    if (IP_TYPE::IP_V6 == m_ipType) {
        addrLen = sizeof(sockaddr_in6) * 2;
    }
      
    string strAddressIP;
    strAddressIP.resize(200);
    int nLocalLen, nRmoteLen;
    LPSOCKADDR pLocalAddr, pRemoteAddr;
    this->m_getAcceptExSockaddrs(perIoData->buffer, DATA_BUFSIZE - addrLen*2,
        addrLen,
        addrLen,
        (SOCKADDR **)&pLocalAddr,
        &nLocalLen,
        (SOCKADDR **)&pRemoteAddr,
        &nRmoteLen);
  
 
    sockaddr_in &addr = *(sockaddr_in *)pRemoteAddr;
    string strIP;
    int port = 0;
    if (IP_TYPE::IP_V6 == m_ipType) {
        strIP.resize(64);
        inet_ntop(AF_INET6, pRemoteAddr, &strIP[0], 64);
        port = ntohs(addr.sin_port);
    }
    else {
        strIP.resize(16);
        inet_ntop(AF_INET, pRemoteAddr, &strIP[0], 16);
        port = ntohs(addr.sin_port);
    }
    SOCKET ns = perIoData->session->sockAccept;

    LOG_INFO("sock %d connect from : %s", ns, strIP.c_str());
   
    postAccept(listenFd);
    if (len > 0) {
        string strRecv(perIoData->DataBuf.buf, len);
        LOG_INFO("len=%d  (%s)", len, strRecv.c_str());
    }
    if (!notify(perIoData->session->sockAccept, IOType::IORead)) {
        LOG_ERROR("fault to notify(%d)", perIoData->session->sockAccept);
    }
    
    auto sockInfo = std::make_shared<SOCKINFO>(); 
    sockInfo->session = perIoData->session;
    initSockInfo(sockInfo, ns, portType, strIP, port);    
    if (PORT_TYPE::SOCKET_HTTPS == portType) {
        sslAcept(ns, sockInfo);
    }    
    inputSockInfo(sockInfo);
    onRecv(perIoData, len);  
    
    return;
}

void CSocketService::postRecv(LPPER_IO_DATA perIoData)
{
    auto recvIO = perIoData->session->recvIO.get();
    recvIO->DataBuf.buf = recvIO->buffer;
    recvIO->DataBuf.len = DATA_BUFSIZE;
    recvIO->type = IOType::IORead;
    DWORD recv_len = 0;
    DWORD flags = 0;   
    if (WSARecv(recvIO->session->sockAccept, &(recvIO->DataBuf),
        1, &recv_len, &flags, (LPWSAOVERLAPPED)recvIO, NULL) == SOCKET_ERROR) {
        if (WSAGetLastError() != WSA_IO_PENDING) {
            //LOG_ERROR("recv error to notify(%d)", perIoData->session->sockAccept);
        }
    }
    return;
}

void CSocketService::prevRecv(LPPER_IO_DATA perIoData, int len)
{
    SOCKET fd = perIoData->session->sockAccept;
    auto it = m_mpSockInfo.find(fd);
    if (m_mpSockInfo.end() == it)
    {
        LOG_INFO("not to find sock %d", fd);
        return;
    }
    string & oldData = perIoData->session->strData;
    if (!oldData.empty()) {
        string strData;
        strData.assign(perIoData->buffer, len);
        oldData += strData;
        len = static_cast<int>(oldData.size());
        memcpy(perIoData->buffer, oldData.data(), len);
        oldData.clear();
    }

    string strPrevBuff;

    it->second->pTimer->reset();
    bool bNext = true;
    if (PORT_TYPE::SOCKET_HTTPS == it->second->channel->portType) {
        auto & param = perIoData->session->sslParam;
        auto & ssl = it->second->channel->ssl_;       
        bNext = false;
        if (!SSL_is_init_finished(ssl)) {  
            bool bFind = true;
            int handshake = 0;
            string strSnd;
            string strRecv(perIoData->buffer, len);
            if (!bioWrite(param->rbio(), strRecv)) {               
                LOG_ERROR("error to call bioWrite");
                bFind = false;
            }
            else {
                handshake = SSL_do_handshake(ssl);
                char buf[DATA_BUFSIZE] = { 0 };                
                int r_len = 0;
                do {
                    r_len = BIO_read(param->wbio(), buf, sizeof(buf));
                    if (r_len < 0) {                       
                        bFind = false;
                        break;
                    }
                    strSnd.append(buf, r_len);
                } while (!BIO_eof(param->rbio()));
            }
            if (!bFind) {
                BIO_reset(param->wbio());
                BIO_reset(param->rbio());
                perIoData->session->strData = strRecv;               
                return;
            }

            perIoData->session->strData.clear();           
            postSnd(perIoData->session->sndIO.get(), strSnd);
            
            if (handshake != 1) {
                int err = SSL_get_error(ssl, handshake);
                switch (err) {
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                    postRecv(perIoData);
                    break;
                default:                   
                    return;
                }
            }
            else {
                postRecv(perIoData);
            }

           
            return;
        }
        perIoData->session->strData.clear();
        it->second->channel->ssl_connect = true;
        bNext = true;
      
        if (bNext) {  
            bool bFind = true;
            string strRecv(perIoData->DataBuf.buf, len);
            if (!bioWrite(param->rbio(), strRecv)) {
                LOG_ERROR("error to call bioWrite");
                bFind = false;
            }
            else if (!sslRead(ssl, strPrevBuff, param->rbio())) { 
                LOG_ERROR("error to call sslRead");
                bFind = false;
            }
            if (!bFind) {
                BIO_reset(param->rbio());
                perIoData->session->strData.clear();
                perIoData->session->strData.assign(perIoData->buffer, len);
                return;
            }
        }
    }
    

    if (bNext) {
        int i = it->second->pos;
        char *pData = it->second->data;
        if (PORT_TYPE::SOCKET_HTTPS == it->second->channel->portType) {
            len = static_cast<int>(strPrevBuff.size());
        }
        int recvLen = len + i;
        if (recvLen <= DATA_BUFSIZE) {
            if (PORT_TYPE::SOCKET_HTTPS == it->second->channel->portType)  {
                memcpy(&pData[i], strPrevBuff.data(), len);
            }
            else {
                memcpy(&pData[i], perIoData->DataBuf.buf, len);
            }
        }
        else {
            LOG_ERROR("error package, check package !!!");
            closeConnect(fd);
            return;
        } 

        it->second->pos = handleRawData(it->second, pData, recvLen);
        if (it->second->pos >= DATA_BUFSIZE)
        {
            LOG_WARNING("error data format to recv....");
            closeConnect(fd);
            return;
        }
    }
    return;
}




void CSocketService::onRecv(LPPER_IO_DATA perIoData, int len)
{    
    if (len > 0) {       
        prevRecv(perIoData, len);
    }
    postRecv(perIoData);    
    
    return;
}

void CSocketService::postSnd(LPPER_IO_DATA perIoData, string & strSnd)
{     
    auto sndIO = perIoData->session->sndIO.get();
    sndIO->type = IOType::IOWrite;
    std::size_t len = strSnd.size();
    memcpy(sndIO->buffer, strSnd.data(), len);
    sndIO->DataBuf.buf = sndIO->buffer;
    sndIO->DataBuf.len = len;
    DWORD send_len = 0;
    DWORD flags = 0;
    if (WSASend(sndIO->session->sockAccept, &(sndIO->DataBuf),
        1, &send_len, flags, (LPWSAOVERLAPPED)sndIO,
        NULL) == SOCKET_ERROR) {
        if (WSAGetLastError() != WSA_IO_PENDING) {
            LOG_ERROR("snd error to notify(%d)", sndIO->session->sockAccept);
        }
    }
    return;
}

void CSocketService::onSnd(LPPER_IO_DATA perIoData, int len)
{    
    SOCKET fd = perIoData->session->sockAccept;
    auto it = m_mpSockInfo.find(fd);
    if (m_mpSockInfo.end() == it)
    {
        LOG_INFO("not to find sock %d", fd);
        return;
    }
    it->second->pTimer->reset();

    if (PORT_TYPE::SOCKET_HTTPS == it->second->channel->portType) {
        if (!SSL_is_init_finished(it->second->channel->ssl_)) {
            return;
        }
        it->second->channel->ssl_connect = true;
    }

    std::shared_ptr<PACKAGE_INFO> pPkgInfo;
    if (getSndPackage(fd, pPkgInfo)) {
        pPkgInfo->pos += len;
        CHandleMsgS->notifyMsg(pPkgInfo);
    }  
    return;
}

#endif


bool CSocketService::doEvent()
{
#ifdef WIN_OS    
    LPOVERLAPPED lpOverlapped = nullptr;
    IOType  ioType = IOType::IOUnknow;
    LPPER_IO_DATA perIoData = nullptr;
    DWORD recvNum = 0;
    BOOL ret = GetQueuedCompletionStatus(m_handlePort, &recvNum, (PULONG_PTR)&ioType, (LPOVERLAPPED*)&lpOverlapped, MAX_SOCKET_WAIT_MS);
    int errNo = GetLastError();
    if (!ret) {
        if (ERROR_NETNAME_DELETED == errNo) {
            return false;
        }
        else if (WAIT_TIMEOUT == errNo) {

        }
    }
    if (lpOverlapped) {

        perIoData = (LPPER_IO_DATA)CONTAINING_RECORD(lpOverlapped, PER_IO_DATA, Overlapped);
        if (IOType::IOAccept == ioType) {
            doAccept(perIoData, recvNum);          
            return true;
        }
        else if (IOType::IOClose == ioType) {           
            return false;
        }

        if (0 == recvNum) {
            closeConnect(perIoData->session->sockAccept);
            
            return true;
        }

        if (perIoData->session->recvIO.get() == perIoData) {           
            onRecv(perIoData, recvNum);
        }
        else if (perIoData->session->sndIO.get() == perIoData) {           
            onSnd(perIoData, recvNum);
        }
    }
#else 
    vector<struct epoll_event> vecEvent(MAXEVENT);
    struct epoll_event *events = &vecEvent[0];
    int size = static_cast<int>(vecEvent.size());
    int nfds = epoll_wait(m_handlePort, events, size, MAX_SOCKET_WAIT_MS);
    for (int i = 0; i < nfds; ++i) {
        if ((events[i].data.fd == m_listenSocketTcp)
            || (events[i].data.fd == m_listenSocketHttp)
            || (events[i].data.fd == m_listenSocketSSL)
            )
        {
            PORT_TYPE portType = PORT_TYPE::SOCKET_HTTP;
            if (m_listenSocketTcp == events[i].data.fd) {
                portType = PORT_TYPE::SOCKET_TCP;
            }
            else if (m_listenSocketSSL == events[i].data.fd) {
                portType = PORT_TYPE::SOCKET_HTTPS;
            }
            doAccept(events[i].data.fd, portType);
        }
        else if (events[i].events & EPOLLIN) {
            SSL * ssl = nullptr;
            PORT_TYPE portType = getSockType(events[i].data.fd);
            if (PORT_TYPE::SOCKET_HTTPS == portType) {
                sslHandleRead(events[i].data.fd);
            }
            else {
                doRecv(events[i].data.fd, portType, ssl);
            }
        }
        else if (events[i].events & EPOLLOUT) {
            SOCKET fd = events[i].data.fd;          
            auto it = m_mpSockInfo.find(fd);
            if (m_mpSockInfo.end() != it) {
                it->second->pTimer->reset();
                bool bSnd = true;
                PORT_TYPE portType = it->second->channel->portType;
                if (PORT_TYPE::SOCKET_HTTPS == portType) {
                    bSnd = it->second->channel->ssl_connect;
                    if (!bSnd) {
                        sslHandleDataWrite(fd, it->second);
                    }
                }
                if (bSnd) {
                    doSnd(fd, portType, it->second->channel->ssl_);
                }
            }
        }
        else {
            string strError = strerror(errno);
            LOG_INFO("EPOLLHUP sock fail %d: %s", errno, strError.c_str());
            closeConnect(events[i].data.fd);
        }
    }
    if (size <= nfds) {
        size += MAXEVENT;
        vecEvent.resize(size);
    }
#endif

    handleTimeover();
    return true;
}




void CSocketService::notifyRecv(std::shared_ptr<PACKAGE_INFO> & pPkgInfo)
{
#ifdef WIN_OS 
   
#else
    postRecv(pPkgInfo->channel->sock);
#endif
}

void CSocketService::splitInfo(vector<string> & vecInfo, string & strData)
{
    int size = static_cast<int>(strData.size());
    if (size <= DATA_BUFSIZE) {
        vecInfo.push_back(strData);
        return;
    }
    int len = 0;
    int pos = 0;
    int num = size / DATA_BUFSIZE + 1;
    for (int i = 0; i < num; i++) {
        len = size - pos;
        if (len > DATA_BUFSIZE) {
            len = DATA_BUFSIZE;
        }
        vecInfo.push_back(strData.substr(pos, len));
        pos += len;
    }
    return;
}

void CSocketService::sslCode(std::shared_ptr<PACKAGE_INFO> & pPkgInfo)
{
#ifdef WIN_OS  
    if (PORT_TYPE::SOCKET_HTTPS == pPkgInfo->channel->portType) {      
        doSplitCode(pPkgInfo, pPkgInfo->strHead);
        doSplitCode(pPkgInfo, pPkgInfo->strResp);
    }
#endif
}

void CSocketService::doSplitCode(std::shared_ptr<PACKAGE_INFO> & pPkgInfo, string & strData)
{
    if (strData.empty()) {
        return;
    }
#ifdef WIN_OS    

    if (!sslWrite(pPkgInfo->channel->ssl_, strData)) {
        LOG_ERROR("error to call sslWrite");
        return;
    }

    if (!bioRead(pPkgInfo->session->sslParam->wbio(), strData)) {
        LOG_ERROR("error to call bioRead");
        return;
    }  

#endif
}


bool CSocketService::sslRead(SSL *ssl, string & strData, BIO *bio)
{
    bool frist = true; 
    strData.clear();
    char buf[DATA_BUFSIZE];
    do {       
        int len = SSL_read(ssl, buf, DATA_BUFSIZE);
        if (len <= 0) {
            int sslerr = SSL_get_error(ssl, len);
            LOG_WARNING("sslerr=%d", sslerr);
            if (frist) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            else 
               return false;
        }
        if (len > 0)       
            strData.append(buf, len);
        frist = false;
       
    } while (!BIO_eof(bio));
    return true;
}


bool CSocketService::sslWrite(SSL *ssl, string & strData)
{
    string::size_type size = strData.size();
    string::size_type i = 0;
    string::size_type pos = 0;
    do {
        pos = SSL_write(ssl, &strData[i], size - i);
        if (pos <= 0) {           
            return false;
        }
        i += pos;
    } while (i < size);
    return true;
    //int nRes = SSL_get_error(m_sslSock, res);
    //SSL_ERROR_WANT_WRITE
}

bool CSocketService::bioWrite(BIO *bio, string & strData)
{   
    int i = 0;
    int size = static_cast<int>(strData.size());
    do {
        int len = BIO_write(bio, &strData[i], size - i);
        if (len <= 0) {
            return false;
        }
        i += len;
    } while (i < size);

    return true;
}

bool CSocketService::bioRead(BIO *bio, string & strData)
{
    strData.clear();
    char buf[DATA_BUFSIZE];    
    do {
        int len = BIO_read(bio, buf, DATA_BUFSIZE);
        if (len <= 0) {
            return false;
        }
        strData.append(buf, len);
    } while (!BIO_eof(bio));
    return true;
}

void CSocketService::notifySnd(std::shared_ptr<PACKAGE_INFO> & pPkgInfo, string & strSnd)
{
#ifdef WIN_OS 
    postSnd(pPkgInfo->session->sndIO.get(), strSnd);
#else
    postSnd(pPkgInfo->channel->sock);
#endif
}

void CSocketService::handleTimeover()
{
    bool bErase = false;
    auto it = m_mpSockInfo.begin();
    for (; it != m_mpSockInfo.end();)
    {
        //LOG_INFO("check socket %d", it->second->sock);
        if (it->second->pTimer->elapsed_seconds() >= MAX_IDLE_SEC)
        {           
            eraseSndMsg(it->first);           
            SSL_free(it->second->channel->ssl_);
            closeConnect(it->second->channel->sock, false);
            m_mpSockInfo.erase(it++);
            bErase = true;
        }
        else
        {
            ++it;
        }
    }

    if (bErase) {
        if (m_mpSockInfo.empty()) {
            map<SOCKET, shared_ptr<SOCKINFO> >().swap(m_mpSockInfo);
        }
    }
    return;
}

void CSocketService::closeConnect(SOCKET fd, bool Erase)
{
    if (Erase)
    {
        eraseSockInfo(fd);
    }

#ifdef WIN_OS 
    shutdown(fd, SD_BOTH);
#endif
    closesocket(fd);
#ifdef LINUX_OS 
    struct epoll_event ev = { 0,{ 0 } };
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    epoll_ctl(m_handlePort, EPOLL_CTL_DEL, fd, &ev);
#endif   
   
}



PORT_TYPE CSocketService::getSockType(SOCKET fd)
{
    PORT_TYPE type = PORT_TYPE::SOCKET_HTTP;
    auto it = m_mpSockInfo.find(fd);
    if (m_mpSockInfo.end() == it)
    {
        LOG_INFO("***** not to find sock %d", fd);
        return type;
    }
    return it->second->channel->portType;
}

void CSocketService::inputSockInfo(shared_ptr<SOCKINFO>  & sockInfo)
{
    eraseSockInfo(sockInfo->channel->sock);
    m_mpSockInfo[sockInfo->channel->sock] = sockInfo;
}

void CSocketService::eraseSockInfo(SOCKET fd)
{
    auto it = m_mpSockInfo.find(fd);
    if (m_mpSockInfo.end() != it)
    {
        
#ifdef WIN_OS 
        if (it->second->pPkgInfo) {
            if (it->second->pPkgInfo->session) {
                releaseSession(it->second->pPkgInfo->session);
            }
        }
#endif
        m_mpSockInfo.erase(it);
        if (m_mpSockInfo.empty()) {
            map<SOCKET, shared_ptr<SOCKINFO>>().swap(m_mpSockInfo);
        }
    }


    return;
}

#ifdef  LINUX_OS 
void CSocketService::doAccept(SOCKET sock, PORT_TYPE portType)
{

    do {       
        sockaddr_in addr;
        struct sockaddr_in6 addr_in6;

        struct sockaddr * addrInfo = (struct sockaddr *)&addr;
        if (IP_TYPE::IP_V6 == m_ipType) {
            addrInfo = (struct sockaddr *)&addr_in6;
        }

        socklen_t len = sizeof(sockaddr);
        SOCKET ns = accept(sock, (sockaddr*)&addrInfo, &len);
        if (-1 == ns)
        {
            if (EMFILE == errno || errno == EINTR) {
                string strError = strerror(errno);
                LOG_WARNING("accept not good error=%d: %s", errno, strError.c_str());
            }
            return;
        }

        setNonblocking(ns);

        int optint = 1;
        setsockopt(ns, SOL_SOCKET, SO_KEEPALIVE, (char *)&optint, sizeof(optint));
        optint = 1;
        setsockopt(ns, IPPROTO_TCP, TCP_NODELAY, (char *)&optint, sizeof(optint));
      
        int port = 0;
        string strIP; 
        if (IP_TYPE::IP_V6 == m_ipType) {
            char buf[64] = { 0 };
            inet_ntop(AF_INET6, &addr_in6.sin6_addr, buf, sizeof(buf));
            strIP = buf;
            port = ntohs(addr_in6.sin6_port);
        }
        else {
            strIP = inet_ntoa(addr.sin_addr);
            port = ntohs(addr.sin_port);
        }        

        LOG_INFO("sock %d connect from : %s ", ns, strIP.c_str());    

        struct epoll_event ev;
        memset(&ev, 0, sizeof(ev));
        if (PORT_TYPE::SOCKET_HTTPS == portType) {
            ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
        }
        else {
            ev.events = EPOLLIN | EPOLLET;
        }
        ev.data.fd = ns;
        if (epoll_ctl(m_handlePort, EPOLL_CTL_ADD, ns, &ev) != 0) {
            string strError = strerror(errno);
            LOG_ERROR("epoll_ctl add sock fail %d: %s", errno, strError.c_str());
            return;
        }

        auto sockInfo = std::make_shared<SOCKINFO>();
        initSockInfo(sockInfo, ns, portType, strIP, port);
        
        if (PORT_TYPE::SOCKET_HTTPS == portType) {
            sslAcept(ns, sockInfo);
        }
        inputSockInfo(sockInfo);

    } while (true);
}

void CSocketService::doSnd(SOCKET fd, PORT_TYPE portType, SSL * ssl)
{
    std::shared_ptr<PACKAGE_INFO> pPkgInfo;
    if (getSndPackage(fd, pPkgInfo)) {
        string strMsg;
        CHandleMsgS->extractSndMsg(strMsg, pPkgInfo);
        if (strMsg.empty())
        {
            postRecv(fd);
            return;
        }
        int len = static_cast<int>(strMsg.size());
        int ret = 0;
        if (PORT_TYPE::SOCKET_HTTPS == portType) {
            ret = SSL_write(ssl, &strMsg[0], len);
        }
        else {
            ret = send(fd, &strMsg[0], len, 0);
        }
        if (0 == ret) {
           closeConnect(fd);
          
        }
        else if (ret > 0) {
            pPkgInfo->pos += ret;
            CHandleMsgS->notifyMsg(pPkgInfo);
        }
    }
    else {
        postRecv(fd);
        LOG_ERROR("not to find the sock %d", fd);
    }
    return;
}

void CSocketService::postRecv(SOCKET fd)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR | EPOLLRDHUP;
    ev.data.fd = fd;
    if (epoll_ctl(m_handlePort, EPOLL_CTL_MOD, fd, &ev) != 0)
    {
        string strError = strerror(errno);
        LOG_ERROR("sndMsg epoll_ctl fail %d: %s", errno, strError.c_str());
    }
    return;
}

void CSocketService::postSnd(SOCKET fd)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLOUT | EPOLLET | EPOLLHUP | EPOLLERR | EPOLLRDHUP;
    ev.data.fd = fd;
    if (epoll_ctl(m_handlePort, EPOLL_CTL_MOD, fd, &ev) != 0)
    {
        string strError = strerror(errno);
        LOG_ERROR("sndMsg epoll_ctl fail %d: %s", errno, strError.c_str());
    }
    return;
}

void CSocketService::doRecv(SOCKET fd, PORT_TYPE portType, SSL * ssl)
{
    auto it = m_mpSockInfo.find(fd);
    if (m_mpSockInfo.end() == it)
    {
        LOG_INFO("not to find sock %d", fd);
        return;
    }
    int ret = 0;
    do {
        it->second->pTimer->reset();
        int i = it->second->pos;
        char *pData = it->second->data;
        if (PORT_TYPE::SOCKET_HTTPS == portType) {
            ret = SSL_read(ssl, &pData[i], DATA_BUFSIZE - i); 
        }
        else {
            ret = recv(fd, &pData[i], DATA_BUFSIZE - i, 0);
        }
        if (0 == ret) {
            closeConnect(fd);
            
            return;
        }
        else if (0 > ret)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                break;
            }
            else if (errno == EINTR)
            {
                continue;
            }
            closeConnect(fd);
            LOG_INFO("close error %d  errMsg:%s", errno, strerror(errno));
            return;
        }
        i += ret;      

        it->second->pos = handleRawData(it->second, pData, i);
        if (it->second->pos >= DATA_BUFSIZE)
        {
            LOG_WARNING("error data rormat to recv....");
            closeConnect(fd);
            return;
        }
    } while (true);
    it->second->pTimer->reset();
    return;
}

#endif


char * CSocketService::headEnd(char *pMsg, string & strFlag, int & recvLen)
{  
    char *pStr = nullptr; 
    for (auto & flagLN : m_liLN) {
        pStr = strstr(pMsg, flagLN.data());
        if (pStr) {
            strFlag = flagLN;        
            break;
        }
    } 

    if (pStr) {   
        bool bFind = false;
        char *pTemp = strstr(pMsg, " ");
        if (pTemp) {            
            string strMothed(pMsg, pTemp - pMsg);
            UTIL_SELF::trimstr(strMothed);
            transform(strMothed.begin(), strMothed.end(), strMothed.begin(), ::toupper);
            auto it = m_mpMothed.find(strMothed);
            if (m_mpMothed.end() != it) {
                bFind = true;
            }
            if (!bFind) {
                LOG_WARNING("not to find the http mothed");
                recvLen = DATA_BUFSIZE;
                pStr = nullptr;
                return pStr;
            }
        }
        else {
            LOG_WARNING("not to find the http mothed");
            pStr = nullptr;
            recvLen = DATA_BUFSIZE;
        }
    }
    return pStr;
}

void CSocketService::handleMothed(std::shared_ptr<PACKAGE_INFO> & pInfo)
{
    string sapceFlag = " ";
    string::size_type pos, next;
    pos = pInfo->strHead.find(sapceFlag);
    if (string::npos == pos)
    {
        return;
    }
    string strMothed = pInfo->strHead.substr(0, pos);
    pos = pInfo->strHead.find_first_not_of(sapceFlag, pos);
    if (string::npos == pos)
    {
        return;
    }

    next = pInfo->strHead.find(sapceFlag, pos);
    if (string::npos == next)
    {
        return;
    }
    string strParam = pInfo->strHead.substr(pos, next - pos);
    UTIL_SELF::trimstr(strParam);
    pInfo->uri = strParam;

    int size = static_cast<int>(strParam.size());
    if (size > 1) {
        int i = size - 1;
        if ('/' == strParam[i]) {
            strParam.pop_back();
        }
    }

    UTIL_SELF::trimstr(strMothed);
    transform(strMothed.begin(), strMothed.end(), strMothed.begin(), ::toupper);

    pInfo->url = strParam;
    pInfo->commandID = HTTP_POST;
    if (GET_MOTHED == strMothed) {
        pInfo->commandID = HTTP_GET;
    }
    getMothedParam(pInfo, strParam);
    //LOG_INFO("mothed %s param %s", strMothed.c_str(), strParam.c_str());
    return;
}

void CSocketService::getMothedParam(std::shared_ptr<PACKAGE_INFO> & pInfo, string & strInfo)
{
    string strFlag = "?";
    string::size_type pos, next;
    pos = strInfo.find(strFlag);
    if (string::npos == pos)
    {
        return;
    }
    string strTemp = strInfo.substr(0, pos);
    pInfo->url = UTIL_SELF::trim(strTemp);

    pos += strFlag.size();
    strFlag = "&";

    string::size_type size = strInfo.size();
    do {
        next = strInfo.find(strFlag, pos);
        if (string::npos == next)
        {          
            getUrlInfo(pInfo, strInfo.substr(pos, size - pos));

            break;
        }
        else
        { 
            getUrlInfo(pInfo, strInfo.substr(pos, next - pos));
            pos = next;
            pos += strFlag.size();
        }
    } while (true);

    return;
}

void CSocketService::getUrlInfo(std::shared_ptr<PACKAGE_INFO> & pInfo, string && strInfo)
{

    auto & mpOriginal = pInfo->mOriginalParam;
    auto & mpInfo = pInfo->mParam;
    string strFlag = "=";
    string::size_type size = strInfo.size();
    string::size_type pos = strInfo.find(strFlag);
    string key = strInfo.substr(0, pos);
    if (key.empty()) {
        LOG_WARNING("empty key(%s)", strInfo.c_str());
        return;
    }
    pos += strFlag.size();
    string value = strInfo.substr(pos, size - pos);
    key = UTIL_SELF::UrlDecode(key);
    mpOriginal[key] = value;
    value = UTIL_SELF::UrlDecode(value);
    UTIL_SELF::trimstr(value);
    mpInfo[key] = value;

    return;
}

void CSocketService::getLNFlag(string & flag, string & strInfo)
{
    string::size_type size = strInfo.size();
    int len = size / 2;
    flag = len > 0 ? strInfo.substr(0, len) : strInfo;
    return;
}

void CSocketService::handleField(std::shared_ptr<PACKAGE_INFO> & pInfo)
{
    std::unordered_map<string, string> & mField = pInfo->mField;
    string & strHead = pInfo->strHead;
    string strLimit = ":";
    string strSpace = " ";
    string & strFlag = pInfo->strCRLF;
    getLNFlag(strFlag, pInfo->strFieldFlag);
    
    string::size_type pos = 0, next = 0;
    pos = strHead.find(strFlag);
    if (string::npos == pos) {
        return;
    }
    pos += strFlag.size();
    do {
        next = strHead.find(strLimit, pos);
        if (string::npos == next) {
            break;
        }
        string key = strHead.substr(pos, next - pos);
        pos = next;
        pos += strLimit.size();
        pos = strHead.find_first_not_of(strSpace, pos);
        if (string::npos == pos) {
            break;
        }
        next = strHead.find(strFlag, pos);
        if (string::npos == next) {
            break;
        }
        string value = strHead.substr(pos, next - pos);
        transform(key.begin(), key.end(), key.begin(), (int(*)(int))tolower);
        if (!value.empty()) {
            mField[key] = value;
        }
        pos = next;
        pos += strFlag.size();
    } while (true);
}

int CSocketService::countPackage(shared_ptr<SOCKINFO> & sockInfo)
{
    int nRet = 0;
    int len = 0;
    for (auto & it : sockInfo->vecBody) {
        len = static_cast<int>(it->size());
        nRet += len;
    }
    return nRet;
}

bool CSocketService::existCache(std::shared_ptr<PACKAGE_INFO> & pPkgInfo)
{
    bool bResult = false;
    if (HTTP_GET == pPkgInfo->commandID) {
        pPkgInfo->strResp.clear();
        //bResult = CMapMngS->getQueryCache(pPkgInfo->uri, pPkgInfo->strResp);
        //if (bResult) {
        //sndMsg(pPkgInfo->sock, pPkgInfo->strResp);
        //}
    }
    return bResult;
}

void CSocketService::initSockInfo(shared_ptr<SOCKINFO> & sockInfo, SOCKET fd,
    PORT_TYPE portType, string & strIP, int port)
{
    sockInfo->channel = std::make_shared<CHANNEL_INFO>();
    SOCKINFO &info = *sockInfo;
    info.channel->ssl_ = nullptr;
    info.channel->ssl_connect = false;
    info.channel->portType = portType;
    info.count = 0;
    info.channel->sock = fd;
    info.ip = strIP;
    info.port = port;
    info.pTimer = std::make_shared<UTIL_SELF::Timer>();
    info.pos = 0;
    info.pkgLen = 0;   
    
#ifdef HTTP_V2 
    info.alpnState = HTTP2_CONNECT_STATE::HTTP2_ALPN_NONE;
    info.h2Mng = std::make_shared<CH2Mng>();
    info.h2RawData = std::make_shared<string>();
#endif

    return;
}

void CSocketService::eraseSndMsg(SOCKET fd)
{
    std::lock_guard<std::mutex>lck(m_sndLock);
    auto it = m_sndMsg.find(fd);
    if (m_sndMsg.end() != it) {
        m_sndMsg.erase(it);
        if (m_sndMsg.empty()) {
            std::map<SOCKET, shared_ptr<PACKAGE_INFO>>().swap(m_sndMsg);
        }
    }
    return;
}

bool CSocketService::getSndPackage(SOCKET fd, std::shared_ptr<PACKAGE_INFO> & pPkgInfo)
{
    bool bResult = false;
    std::lock_guard<std::mutex>lck(m_sndLock);
    auto it = m_sndMsg.find(fd);
    if (m_sndMsg.end() != it) {
        bResult = true;
        pPkgInfo = it->second;
        m_sndMsg.erase(it);
        if (m_sndMsg.empty()) {
            std::map<SOCKET, shared_ptr<PACKAGE_INFO>>().swap(m_sndMsg);
        }
    }
    return bResult;
}

void CSocketService::inputSndPackage(SOCKET fd, std::shared_ptr<PACKAGE_INFO> & pPkgInfo)
{
    std::lock_guard<std::mutex>lck(m_sndLock);
    m_sndMsg[fd] = pPkgInfo;
}

bool CSocketService::chunkedMsg(shared_ptr<SOCKINFO> & sockInfo, bool & bChunked, int & retLen)
{
    int num = countPackage(sockInfo);
    string strChunk;
    strChunk.resize(num);
    std::size_t i = 0;
    std::size_t len = 0;
    for (auto & it : sockInfo->vecBody) {
        len = static_cast<std::size_t>(it->size());
        memcpy(&strChunk[i], it->data(), len);
        i += len;
    }
    string strEndFlag = "0" + sockInfo->pPkgInfo->strFieldFlag;
    string::size_type end = strChunk.find(strEndFlag);
    if (string::npos == end) {
        return false;
    }
    bChunked = true;
    end += strEndFlag.size();
    retLen = (int)end;

    sockInfo->strChunkBody.clear();

    string & crlnflag = sockInfo->pPkgInfo->strCRLF;    
    string::size_type pos = 0;
    string::size_type last = end;
    string::size_type begin = 0;
    do {
        len = 0;
        end = strChunk.find(crlnflag, begin);
        if (string::npos == end) {
            break;
        }
        pos = strChunk.find(";", begin);
        if (string::npos != pos) {
            if (end < pos) {
                pos = end;
            }
        }
        else {
            pos = end;
        }

        stringstream ss;
        ss.str(strChunk.substr(begin, pos - begin).c_str());
        ss >> std::hex >> len;
        if (0 == len) {
            if ((strEndFlag.size() + end) >= last) {
                //it's over to parse thunk block;
                break;
            }
        }

        end += sockInfo->pPkgInfo->strCRLF.size();
        sockInfo->strChunkBody += strChunk.substr(end, len);
        end += len;
        end += sockInfo->pPkgInfo->strCRLF.size();

        begin = end;
        if (begin >= last) {
            break;
        }
    } while (true);
   
    return true;
}

bool CSocketService::fowardMsg(shared_ptr<SOCKINFO> & sockInfo, bool & bPost, bool & bChunked, int & len)
{
    len = -1;
    bool bResult = true;
    std::shared_ptr<PACKAGE_INFO> & pInfo = sockInfo->pPkgInfo;
    if (HTTP_POST == pInfo->commandID) {
        bPost = true;
        auto it = pInfo->mField.find("content-length");
        if (pInfo->mField.end() != it) {
            len = atoi(it->second.c_str());
            if (pInfo->bodyLen < len) {
                bResult = false;
            }
        }
        else {
            it = pInfo->mField.find("transfer-encoding");
            if (pInfo->mField.end() != it) {
                string strChunked = it->second;
                UTIL_SELF::trimstr(strChunked);
                if ("chunked" == strChunked) {
                    return chunkedMsg(sockInfo, bChunked, len);
                }
            }
        }
    }
    return bResult;
}

int  CSocketService::handleRawDataHttp(shared_ptr<SOCKINFO> & sockInfo, char *pMsg, int recvLen)
{
    int ret = 0;
    string strExtData;
    do {
        ret = handleDataHttp(sockInfo, pMsg, recvLen);
        if (sockInfo->extData.empty()) {
            break;
        }   
        strExtData = sockInfo->extData;
        pMsg = reinterpret_cast<char*>(&strExtData[0]);
        recvLen = static_cast<int>(strExtData.size());
        LOG_INFO("more pakcage to handle...");       
    } while (true);
    return ret;
}


int CSocketService::handleDataHttp(shared_ptr<SOCKINFO> & sockInfo, char *pMsg, int recvLen)
{
#ifdef HTTP_V2
    if (PORT_TYPE::SOCKET_HTTPS == sockInfo->channel->portType) {
        string strMsg(pMsg, recvLen);
        return sockInfo->h2Mng->http2RawData(sockInfo, strMsg);
    }
#endif

    bool bSurport = true; //post get
    int nRet = 0;
    if (sockInfo->pkgLen <= 0)
    {
        string strFieldFlag;
        auto pStr = headEnd(pMsg, strFieldFlag, recvLen);
        if (!pStr)
        {
            return recvLen;
        }

        int offsetLen = static_cast<int>(strFieldFlag.size());
        int headLen = pStr - pMsg;   
        sockInfo->pPkgInfo = std::make_shared<PACKAGE_INFO>();
        sockInfo->pPkgInfo->curPath = CMngConfigS->getXmlPtr()->getPath();
        sockInfo->pPkgInfo->strHead.assign(pMsg, headLen);
        sockInfo->pPkgInfo->strFieldFlag = strFieldFlag;
        sockInfo->pPkgInfo->bodyLen = recvLen - headLen - offsetLen;
        sockInfo->pPkgInfo->bFCGIResp = false;
        pStr += offsetLen;
        if (sockInfo->pPkgInfo->bodyLen > 0) {
            auto pkgTemp = std::make_shared<string>();
            pkgTemp->assign(pStr, sockInfo->pPkgInfo->bodyLen);
            sockInfo->vecBody.push_back(pkgTemp);
        }

        handleMothed(sockInfo->pPkgInfo);
        handleField(sockInfo->pPkgInfo);
        sockInfo->pkgLen = recvLen;

    }
    else {
        auto pkgTemp = std::make_shared<string>();
        pkgTemp->assign(pMsg, recvLen);
        sockInfo->vecBody.push_back(pkgTemp);
        sockInfo->pPkgInfo->bodyLen += recvLen;
        sockInfo->pkgLen += recvLen;
    }
    bool bChunked = false;
    bool bPost = false;
    int postLen = -1;
    bool bTE = fowardMsg(sockInfo, bPost, bChunked, postLen);

    if (bTE) {
        if ((sockInfo->pPkgInfo->bodyLen > 0) && (!bChunked)) {
            string & strRef = sockInfo->pPkgInfo->strBody;
            strRef.clear();
            strRef.resize(sockInfo->pPkgInfo->bodyLen);
            int i = 0;
            int len = 0;
            for (auto & it : sockInfo->vecBody) {
                len = static_cast<int>(it->size());
                memcpy(&strRef[i], it->data(), len);
                i += len;
            }
            sockInfo->vecBody.clear();
        }
        if (bPost) {
            if ((postLen < sockInfo->pPkgInfo->bodyLen) && (postLen > 0) && (!bChunked)) {

                sockInfo->extData.clear();
                sockInfo->extLen = sockInfo->pPkgInfo->bodyLen - postLen;
                string & refExt = sockInfo->extData;
                refExt.resize(sockInfo->extLen);

                string & refBoyd = sockInfo->pPkgInfo->strBody;
                memcpy(&refExt[0], &refBoyd[postLen], sockInfo->extLen);
                refBoyd.erase(postLen, sockInfo->extLen);
            }  
            if (bChunked) {
                if (sockInfo->strChunkBody.size() > 0)
                    sockInfo->pPkgInfo->strBody = sockInfo->strChunkBody;
                sockInfo->strChunkBody.clear();
            }
        }
        else if (HTTP_GET == sockInfo->pPkgInfo->commandID) {

            sockInfo->pPkgInfo->vecFormData.clear();
            sockInfo->pPkgInfo->strBody.clear();
        }
        else {
            bSurport = false;
            LOG_WARNING("no surport mothed %d", sockInfo->pPkgInfo->commandID);
        }

        sockInfo->pkgLen = 0;
        sockInfo->pPkgInfo->bodyLen = 0;

        if (bSurport) {
#ifdef WIN_OS 
            sockInfo->pPkgInfo->session = sockInfo->session;
#endif
           
            sockInfo->pPkgInfo->ip = sockInfo->ip;
            sockInfo->pPkgInfo->channel = sockInfo->channel;

            if (!existCache(sockInfo->pPkgInfo)) {
                CHandleMsgS->inputMsg(sockInfo->pPkgInfo);
            }
        }
        else {
            nRet = DATA_BUFSIZE;
            
        }

        //http 1.1
    }   
    return nRet;
}

int CSocketService::handleRawData(shared_ptr<SOCKINFO> & sockInfo, char *pMsg, int recvLen)
{
    int nRet = 0;  
    if ((PORT_TYPE::SOCKET_HTTP == sockInfo->channel->portType)
        || (PORT_TYPE::SOCKET_HTTPS == sockInfo->channel->portType))  {
        return handleRawDataHttp(sockInfo, pMsg, recvLen);
    }

    return nRet;
}

int CSocketService::sslServernameCB(SSL *s, int *ad, void *arg) {
    if (s == NULL) {
        LOG_ERROR("error!! ssl_servername_cb");
        return SSL_TLSEXT_ERR_NOACK;
    }
    string key;
    SSL_CTX * ctx = nullptr;
    const char* servername = SSL_get_servername(s, TLSEXT_NAMETYPE_host_name);
    if (servername) {
        LOG_INFO("ServerName: %s", servername);
        key = servername;
        ctx = CHandleMsgS->getCtx(key);
    } 
    else {
        ctx = CHandleMsgS->getDefaultCtx();
    }
    if (ctx) {       
        SSL_set_SSL_CTX(s, ctx);
        SSL_set_verify(s, SSL_CTX_get_verify_mode(ctx),
        SSL_CTX_get_verify_callback(ctx));
        SSL_set_verify_depth(s, SSL_CTX_get_verify_depth(ctx));
        SSL_set_options(s, SSL_CTX_get_options(ctx));  
    }
    return SSL_TLSEXT_ERR_OK;
}

SSL_CTX * CSocketService::getCtx(string & key)
{
    SSL_CTX * ctx = nullptr;
    auto it = m_mpCtxSSL.find(key);
    if (m_mpCtxSSL.end() != it) {
        ctx = it->second;
    }
    else {
        ctx = g_sslTemp;
        LOG_ERROR("not to find %s", key.c_str());
    }

    return ctx;
}


int CSocketService::selectNextProtocol(uint8_t **out, uint8_t *outlen, const uint8_t *in,
    unsigned int inlen, const char *key, unsigned int keylen)
{
    unsigned int i;
    for (i = 0; i + keylen <= inlen; i += (unsigned int)(in[i] + 1)) {
        if (memcmp(&in[i], key, keylen) == 0) {
            *out = (unsigned char *)&in[i + 1];
            *outlen = in[i];
            return 0;
        }
    }
    return -1;
}

int CSocketService::alpnSelectProtoCB(SSL *ssl, const uint8_t **out, uint8_t *outlen,
    const uint8_t *in, unsigned int inlen, void *arg)
{ 
    unsigned int            srvlen;
    unsigned char          *srv;
    srv =  (unsigned char *)HTTP2_PROTO_ALPN HTTP2_HTTP_1_1_ALPN;
    srvlen = sizeof(HTTP2_PROTO_ALPN HTTP2_HTTP_1_1_ALPN) - 1;
    if (SSL_select_next_proto((unsigned char **)out, outlen, srv, srvlen,in, inlen)
        != OPENSSL_NPN_NEGOTIATED)
    {
        return SSL_TLSEXT_ERR_NOACK;
    }   
    return SSL_TLSEXT_ERR_OK;
}

bool CSocketService::initSSL()
{
    
#if OPENSSL_VERSION_NUMBER >= 0x10100003L
    if (OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CONFIG, NULL) == 0) {

    }

    /*
    * OPENSSL_init_ssl() may leave errors in the error queue
    * while returning success
    */

    ERR_clear_error();
#else

    OPENSSL_config(NULL);
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

#endif
    bool bFirst = true;
    string curPath = CMngConfigS->getXmlPtr()->getPath();
    vector<std::shared_ptr<SERVICE_CONFIG_INFO>>  vecConfig;
    CMngConfigS->getJsonPtr()->getAllConfig(vecConfig);
    for (auto & config : vecConfig) {
        if (config->https) {
            HTTPS_INFO & httpsInfo = config->https_config;
            string strPath = UTIL_SELF::getAbsolutePath(httpsInfo.path, curPath);
            string rootFile = strPath + httpsInfo.fileRoot;
            if (!UTIL_SELF::isExistFile(rootFile)) {
                LOG_ERROR("not exist file (%s)", rootFile.c_str());
                return false;
            }
            string caFile = strPath + httpsInfo.fileCa;
            if (!UTIL_SELF::isExistFile(caFile)) {
                LOG_ERROR("not exist file (%s)", caFile.c_str());
                return false;
            }
            string keyFile = strPath + httpsInfo.fileKey;
            if (!UTIL_SELF::isExistFile(keyFile)) {
                LOG_ERROR("not exist file (%s)", keyFile.c_str());
                return false;
            }

            auto  sslCtx = SSL_CTX_new(TLS_server_method());
            if (SSL_CTX_set_cipher_list(sslCtx, DEFAULT_CIPHER_LIST) == 0) {
                LOG_ERROR("%s",ERR_error_string(ERR_get_error(), nullptr));
                return false;
            }
            SSL_CTX_set_options(sslCtx,
                SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
                SSL_OP_NO_COMPRESSION |
                SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
            EC_KEY *ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
            if (!ecdh) {
                LOG_ERROR("EC_KEY_new_by_curv_name failed: %s",ERR_error_string(ERR_get_error(), NULL));
            }
            SSL_CTX_set_tmp_ecdh(sslCtx, ecdh);
            EC_KEY_free(ecdh);

            if (0 >= SSL_CTX_load_verify_locations(sslCtx, rootFile.c_str(), NULL)) {
                LOG_ERROR("fault to call SSL_CTX_load_verify_locations (%s)", rootFile.c_str());
                return false;
            }
            if (0 >= SSL_CTX_use_certificate_file(sslCtx, caFile.c_str(), SSL_FILETYPE_PEM)) {
                LOG_ERROR("fault to SSL_CTX_use_certificate_file (%s)", caFile.c_str());
                return false;
            }
            if (0 >= SSL_CTX_use_PrivateKey_file(sslCtx, keyFile.c_str(), SSL_FILETYPE_PEM)) {
                LOG_ERROR("fault to SSL_CTX_use_PrivateKey_file (%s)", keyFile.c_str());
                return false;
            }
            if (0 >= SSL_CTX_check_private_key(sslCtx)) {
                LOG_ERROR("SSL_CTX_check_private_key failed");
                return false;
            }

            m_mpCtxSSL[config->server_name] = sslCtx;
            if (bFirst) {
                g_sslTemp = sslCtx;
                bFirst = false;
            }
                if (0 >= SSL_CTX_set_tlsext_servername_callback(sslCtx, CSocketService::sslServernameCB)) {
                    LOG_ERROR(" OpenSSL library which has no tlsext support");
                    return false;
                }
#ifdef HTTP_V2
                SSL_CTX_set_alpn_select_cb(sslCtx, alpnSelectProtoCB, NULL);
#endif
               

        }
    }

    return true;
}

void CSocketService::sslHandleRead(SOCKET fd)
{    
    auto it = m_mpSockInfo.find(fd);
    if (m_mpSockInfo.end() == it) {

        LOG_ERROR("error to be happended about openssl %d", fd);
        return;
    }
    if (it->second->channel->ssl_connect) {        
        sslHandleDataRead(fd, it->second);
    }
    else {
        sslHandleHandshake(fd, it->second);
    }
}

void CSocketService::sslHandleDataWrite(SOCKET fd, shared_ptr<SOCKINFO> & info)
{ 
#ifdef WIN_OS 


#else
    if (info->channel->ssl_connect) {

        struct epoll_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR | EPOLLRDHUP;
        ev.data.fd = fd;
        int ret = epoll_ctl(m_handlePort, EPOLL_CTL_MOD, fd, &ev);
        if (0 != ret) {
            string strError = strerror(errno);
            LOG_ERROR("epoll_ctl add sock fail %d: %s", errno, strError.c_str());
        }
        return;
    }
    sslHandleHandshake(fd, info);
#endif
}

void CSocketService::sslHandleDataRead(SOCKET fd, shared_ptr<SOCKINFO> & info)
{
#ifdef HTTP_V2
    if (HTTP2_CONNECT_STATE::HTTP2_ALPN_NONE == info->alpnState) {
        const unsigned char *alpn = NULL;
        unsigned int alpnlen = 0;
        SSL *ssl = info->channel->ssl_;
        SSL_get0_alpn_selected(ssl, &alpn, &alpnlen);
        if (alpnlen >= 0) {
            string strAlpn((char*)alpn, alpnlen);
            if (HTTP2_H2 == strAlpn) {
                info->alpnState = HTTP2_CONNECT_STATE::HTTP2_ALPN_PREFACE;
                info->h2Mng->preface(info);
            }
        }
    }
#endif
#ifdef WIN_OS 


#else
    doRecv(fd, PORT_TYPE::SOCKET_HTTPS, info->channel->ssl_);
#endif
}

void CSocketService::sslHandleHandshake(SOCKET fd, shared_ptr<SOCKINFO> & info)
{
#ifdef WIN_OS 


#else
    int ret = SSL_do_handshake(info->channel->ssl_);
    if (1 == ret) {        
        info->channel->ssl_connect = true;

        return;
    }
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    int err = SSL_get_error(info->channel->ssl_, ret);
    if (err == SSL_ERROR_WANT_WRITE) {
        ev.events = EPOLLOUT | EPOLLET | EPOLLHUP | EPOLLERR | EPOLLRDHUP;
        ev.data.fd = fd;
        int ret = epoll_ctl(m_handlePort, EPOLL_CTL_MOD, fd, &ev);
        if (0 != ret) {
            string strError = strerror(errno);
            LOG_ERROR("epoll_ctl add sock fail %d: %s", errno, strError.c_str());
        }
        //LOG_INFO("SSL_ERROR_WANT_WRITE");
    }
    else if (err == SSL_ERROR_WANT_READ) {

        ev.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR | EPOLLRDHUP;
        ev.data.fd = fd;
        int ret = epoll_ctl(m_handlePort, EPOLL_CTL_MOD, fd, &ev);
        if (0 != ret) {
            string strError = strerror(errno);
            LOG_ERROR("epoll_ctl add sock fail %d: %s", errno, strError.c_str());
        }
        //LOG_INFO("SSL_ERROR_WANT_READ");
    }
    else {
        LOG_INFO("SSL_do_handshake return %d error %d errno %d msg %s\n", ret, err, errno, strerror(errno));
    }
#endif
    return;
}

void CSocketService::sslAcept(SOCKET fd, shared_ptr<SOCKINFO> & info)
{
    info->channel->ssl_ = SSL_new(g_sslTemp);
    assert(info->channel->ssl_ != nullptr);

#ifdef WIN_OS 
    info->session->sslParam = std::make_shared<CSslParam>(info->channel->ssl_);
#else
    int ret = SSL_set_fd(info->channel->ssl_, fd);
    if (ret <= 0) {
        string strError = strerror(errno);
        LOG_ERROR("epoll_ctl add sock fail %d: %s", errno, strError.c_str());
    }
#endif     
    SSL_set_accept_state(info->channel->ssl_); 
    return;
}


