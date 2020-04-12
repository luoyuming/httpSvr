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


#include "handleMsg.h"
#include "configMng.h"
#include "dispatch.h"
#include "h2Mng.h"

CHandleMsg::CHandleMsg()
{
    m_stop.store(false);
    m_sockService = std::make_shared<CSocketService>();
    m_asynFile = std::make_shared<CAsynFile>();
    m_QAsyn = std::make_shared<SafeQueue<std::shared_ptr<PACKAGE_INFO>>>();
}

void CHandleMsg::Quit()
{

}

void CHandleMsg::stop()
{
    m_stop.store(true);
    m_QDispatch->exit();
    m_QAsyn->exit();
}

bool CHandleMsg::init()
{    
    if (!CMngConfigS->init()) {
        LOG_ERROR("fault to init config");
        return false;
    }    
    auto xmlPtr = CMngConfigS->getXmlPtr();

    int queueLen = std::atoi(xmlPtr->getSystemXml().queue_capacity.c_str());
    m_QDispatch = std::make_shared<SafeQueue<std::shared_ptr<PACKAGE_INFO>>>(queueLen);

    if (!m_sockService->init(xmlPtr->getSystemXml())) {

        LOG_ERROR("fault to init socket");
        return false;
    }

    return true;
}

void CHandleMsg::starSvr()
{
    buildThread();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
}

void CHandleMsg::buildThread()
{
    int coreNum = thread::hardware_concurrency();
    LOG_INFO("hardware_concurrency %d", coreNum);

    int threadNum = CMngConfigS->getXmlPtr()->getThreadEvnetNum();
    for (int i = 0; i < threadNum; i++) {
        std::thread threadHandleEvent(&CHandleMsg::threadHandleEvent, this);
        m_threadPool.push_back(std::move(threadHandleEvent));
    }

    std::thread threadHandleSocket(&CHandleMsg::threadHandleSocket, this);
    m_threadPool.push_back(std::move(threadHandleSocket));
    std::thread threadHandleAsyn(&CHandleMsg::threadHandleAsyn, this);
    m_threadPool.push_back(std::move(threadHandleAsyn));

}

void CHandleMsg::join()
{
    for (std::thread & thread : m_threadPool) {
        if (thread.joinable()) {
            thread.join();		
        }
    }
    LOG_INFO("join to finish....");
}

void CHandleMsg::threadHandleSocket()
{
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    string threadID = oss.str();
    LOG_INFO("threadHandleSocket begin thread_id %s", threadID.c_str());
    while (!m_stop) {
        if (!m_sockService->doEvent()) {
            break;
        }
    }
    LOG_INFO("threadHandleSocket end thread_id %s", threadID.c_str());
    return;
}

void CHandleMsg::threadHandleEvent()
{
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    string threadID = oss.str();
    LOG_INFO("threadHandleEvent begin thread_id %s", threadID.c_str());
    
    auto dispatch = std::make_shared<CDispatch>();    
    while (!m_stop) {   
        std::shared_ptr<PACKAGE_INFO> pPkgInfo;
        if (m_QDispatch->pop_wait(&pPkgInfo)) {           
            dispatch->dispatchInfo(*pPkgInfo);           
            handleResp(pPkgInfo);
        }
        if (m_QDispatch->exited()) {
            break;
        }
    }
    LOG_INFO("threadHandleEvent end thread_id %s", threadID.c_str());
    return;
}

void CHandleMsg::threadHandleAsyn()
{
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    string threadID = oss.str();
    LOG_INFO("threadHandleAsyn begin thread_id %s", threadID.c_str());
    int waitMS = MAX_ASYN_WAIT_MS;
    while (!m_stop) {
        std::shared_ptr<PACKAGE_INFO> pPkgInfo;
        if (m_QAsyn->pop_wait(&pPkgInfo, waitMS)) {
            m_asynFile->handleAsynFile(pPkgInfo);
        }
        
        m_asynFile->asynReadFile();
        m_asynFile->checkAsynReadStatus();
        waitMS = m_asynFile->getWaitTime();
        if (m_QAsyn->exited()) {
            break;
        }
    }

    LOG_INFO("threadHandleAsyn end thread_id %s", threadID.c_str());
    return;
}

void CHandleMsg::inputMsg(std::shared_ptr<PACKAGE_INFO> & pkg)
{
    while (!m_QDispatch->push(pkg)) {
        if (m_QDispatch->exited()) {          
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        LOG_WARNING("QDispatch is sleeping ...");
    }
    return;
}

bool CHandleMsg::getSndMsg(string & strMsg, string & strData, int pos) {
    int size = static_cast<int>(strData.size());
    int len = size - pos;
    if (len > DATA_BUFSIZE) {
        len = DATA_BUFSIZE;
    }
    strMsg.resize(len);
    memcpy(&strMsg[0], &strData[pos], len);
    return true;
}

bool CHandleMsg::finishToSnd(std::shared_ptr<PACKAGE_INFO> & pPkgInfo)
{
    bool bResult = false;
    int headLen = static_cast<int>(pPkgInfo->strHead.size());
    int bodyLen = static_cast<int>(pPkgInfo->strResp.size());

    if (pPkgInfo->pos >= (headLen + bodyLen)) {

        bResult = true;
    }
    return bResult;
}

void CHandleMsg::extractSndMsg(string & strMsg, std::shared_ptr<PACKAGE_INFO> & pPkgInfo)
{   
    int headLen = static_cast<int>(pPkgInfo->strHead.size());
    int bodyLen = static_cast<int>(pPkgInfo->strResp.size());
    
    if (pPkgInfo->pos >= (headLen + bodyLen)) {
       
        return;
    }

    if (pPkgInfo->pos < headLen) {
        getSndMsg(strMsg, pPkgInfo->strHead, pPkgInfo->pos);
    }
    else {        
        getSndMsg(strMsg, pPkgInfo->strResp, pPkgInfo->pos-headLen);
    }
    return;
}

void CHandleMsg::notifyMsg(std::shared_ptr<PACKAGE_INFO> & pPkgInfo)
{   
    if (finishToSnd(pPkgInfo)) {
        m_sockService->notifyRecv(pPkgInfo);
        return;
    }

    string strMsg;
#ifdef WIN_OS 
    extractSndMsg(strMsg, pPkgInfo);   
    if (strMsg.empty())
    {
        LOG_WARNING("package is empty to extract");
        return;
    }
#endif   
    SOCKET fd = pPkgInfo->channel->sock;
    m_sockService->inputSndPackage(fd, pPkgInfo);
    m_sockService->notifySnd(pPkgInfo, strMsg);
    return;
}

void CHandleMsg::h2BuildBodyFrame(std::shared_ptr<PACKAGE_INFO> & pPkgInfo)
{
    std::uint32_t stream_ind = pPkgInfo->h2Session->m_selfStream;
    vector<string> vecData;
    auto h2Session = pPkgInfo->h2Session;
    const char* buf = pPkgInfo->strResp.data();
    std::uint32_t len = static_cast<std::uint32_t>(pPkgInfo->strResp.size());
    std::uint32_t sent_len = 0;
    while (true) {
        std::uint32_t pre_send_len = ((len - sent_len) > HTTP2_MAX_FRAME_LEN) ? HTTP2_MAX_FRAME_LEN : (len - sent_len);
        //pre_send_len = h2Session->GetPeerWindowSize() > pre_send_len ? pre_send_len : h2Session->GetPeerWindowSize();     
        int total = pre_send_len + HTTP2_FRAME_HEAD_LEN;
        string strDataFrame;
        strDataFrame.resize(total);
       
        HTTP2_FRAME * http2_frm_data = (HTTP2_FRAME *)&strDataFrame[0];
        http2_frm_data->length.len24 = htonl(pre_send_len) >> 8;
        http2_frm_data->type = HTTP2_FRAME_TYPE_DATA;
        http2_frm_data->flags = HTTP2_FRAME_FLAG_UNSET;
        http2_frm_data->r = HTTP2_FRAME_R_UNSET;

        http2_frm_data->identifier = htonl(stream_ind) >> 1;

      
        memcpy(&strDataFrame[HTTP2_FRAME_HEAD_LEN], buf + sent_len, pre_send_len);

        vecData.push_back(strDataFrame);

        sent_len += pre_send_len;
        if (sent_len >= len)
            break;
    }

    int size = static_cast<int>(vecData.size());
    if (size < 0) {
        LOG_ERROR("error");
    }
    else {
        string & lastFrame = vecData[size - 1];
        HTTP2_FRAME * http2_frm_data = (HTTP2_FRAME *)&lastFrame[0];
        http2_frm_data->flags = HTTP2_FRAME_FLAG_END_STREAM;

        int total = 0;
        for (auto & str : vecData) {
            total += static_cast<int>(str.size());
        }
        string & dataFrame = pPkgInfo->strResp;
        dataFrame.clear();
        dataFrame.resize(total);
        int i = 0;
        for (auto & str : vecData) {
            memcpy(&dataFrame[i], str.data(), str.size());
            i += static_cast<int>(str.size());
        }

        LOG_INFO("data frame is over!!");
    }

    return;
}

void CHandleMsg::h2BuildHeadFrame(std::shared_ptr<PACKAGE_INFO> & pPkgInfo)
{
    pPkgInfo->strHead.clear();
    string & headFrame = pPkgInfo->strHead;

    string  strGmt;
    UTIL_SELF::getGmtTime(strGmt);
      
    map<int, pair<string, string> > staticTable;
    pPkgInfo->h2Mng->getStaticTable(staticTable);
    auto hpack(std::make_unique<hpack>());
   
    hpack->buildField(":status", "200", staticTable);
    hpack->buildField("server", "httpSvr", staticTable);
    hpack->buildField("date", strGmt, staticTable);
    hpack->buildField("vary", "Accept-Encoding", staticTable);
    hpack->buildField("connection", "keep-alive", staticTable);

    string strTemp = "timeout =" + std::to_string(MAX_IDLE_SEC);
    hpack->buildField("keep-alive", strTemp, staticTable);
    hpack->buildField("content-length", std::to_string(pPkgInfo->strResp.size()), staticTable);
    if (!pPkgInfo->contentEncoding.empty()) {
       
        strTemp = pPkgInfo->contentEncoding;
        string k, v;
        UTIL_SELF::getKV(k, v, strTemp);
        transform(k.begin(), k.end(), k.begin(), (int(*)(int))tolower);
        hpack->buildField(k, v, staticTable);

        strTemp = pPkgInfo->contentType;    
        UTIL_SELF::getKV(k, v, strTemp);
        transform(k.begin(), k.end(), k.begin(), (int(*)(int))tolower);
        hpack->buildField(k, v, staticTable);
    }

    int payloadLen = hpack->getEncodeSize();
    int total = payloadLen + HTTP2_FRAME_HEAD_LEN;
    headFrame.resize(total);
    hpack->copyField(&headFrame[HTTP2_FRAME_HEAD_LEN]);

    std::uint32_t stream_ind = pPkgInfo->h2Session->m_selfStream;
    std::uint32_t frame_len = (std::uint32_t)payloadLen;

    HTTP2_FRAME * http2_frm_hdr = (HTTP2_FRAME *)&headFrame[0];
    http2_frm_hdr->length.len24 = htonl(frame_len) >> 8;
    http2_frm_hdr->type = HTTP2_FRAME_TYPE_HEADERS;

    http2_frm_hdr->flags = HTTP2_FRAME_FLAG_END_HEADERS;
    http2_frm_hdr->r = HTTP2_FRAME_R_UNSET;

    http2_frm_hdr->identifier = htonl(stream_ind) >> 1;

    LOG_INFO("head frame is over!!");

    return;
}

void CHandleMsg::buildHeadResp(string & strInfo, std::shared_ptr<PACKAGE_INFO> & pPkgInfo)
{
    string  strGmt;
    UTIL_SELF::getGmtTime(strGmt);
    strInfo = "HTTP/1.1 200 OK\r\nServer: httpSvr\r\nDate: ";
    strInfo += strGmt;
    strInfo += CRLN_FLAG;
    strInfo += "Vary: Accept-Encoding\r\nConnection: keep-alive\r\n";
    strInfo += "Keep-Alive: timeout=";
    strInfo += std::to_string(MAX_IDLE_SEC);
    strInfo += CRLN_FLAG;
    if (!pPkgInfo->bFCGIResp) {
        strInfo += ContentLength;
        strInfo += std::to_string(pPkgInfo->strResp.size());
        strInfo += CRLN_FLAG;
        if (!pPkgInfo->contentEncoding.empty()) {
            strInfo += pPkgInfo->contentEncoding;
            strInfo += CRLN_FLAG;
        }
        strInfo += pPkgInfo->contentType;
        strInfo += HEAD_END_FLAG;
    }
    return;
}

void CHandleMsg::resetMsg(std::shared_ptr<PACKAGE_INFO> & pPkgInfo)
{
    pPkgInfo->vecFormData.clear();
    pPkgInfo->mField.clear();
    pPkgInfo->mParam.clear();
    pPkgInfo->mOriginalParam.clear();
    pPkgInfo->strBody.clear();
    pPkgInfo->strHead.clear();
}


void CHandleMsg::handleResp(std::shared_ptr<PACKAGE_INFO> & pPkgInfo)
{       

    if (!pPkgInfo->strResp.empty()) {  
        resetMsg(pPkgInfo);
        pPkgInfo->pos = 0;

#ifdef HTTP_V2 
        if ((PORT_TYPE::SOCKET_HTTPS == pPkgInfo->channel->portType)
            && (pPkgInfo->bH2RespHead)) {
            h2BuildHeadFrame(pPkgInfo);
            h2BuildBodyFrame(pPkgInfo);
        }
#else
        buildHeadResp(pPkgInfo->strHead, pPkgInfo);
#endif

       
#ifdef WIN_OS 
        m_sockService->sslCode(pPkgInfo);
#endif

        notifyMsg(pPkgInfo);
    }
    else if (!pPkgInfo->fileNameAsyn.empty()) {       
        inputAsyn(pPkgInfo);
    }
    return;
}

void CHandleMsg::inputAsyn(std::shared_ptr<PACKAGE_INFO> & pkg)
{
    m_QAsyn->push(pkg);
}

SSL_CTX * CHandleMsg::getCtx(string & key)
{
    return m_sockService->getCtx(key);
}

SSL_CTX * CHandleMsg::getDefaultCtx()
{
    return m_sockService->g_sslTemp;
}

std::shared_ptr<CSocketService> CHandleMsg::getService()
{
    return m_sockService;
}