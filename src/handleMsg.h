#pragma once
#include "singleton.h"
#include "common.h"
#include "util.h"
#include "log.h"
#include "socketService.h"
#include "taskQueue.h"
#include "asynFile.h"

static const int MAX_QUEUE_WAIT_MS = 50;  

class CHandleMsg : public SingletionEX<CHandleMsg>
{
    SINGLETON_INIT_EX(CHandleMsg);
    CHandleMsg();
public:
    bool init();
    void starSvr();
    void join();
    void stop();


    void inputMsg(std::shared_ptr<PACKAGE_INFO> & pkg);
    void notifyMsg(std::shared_ptr<PACKAGE_INFO> & pPkgInfo);
    void extractSndMsg(string & strMsg, std::shared_ptr<PACKAGE_INFO> & pPkgInfo);
    void resetMsg(std::shared_ptr<PACKAGE_INFO> & pPkgInfo);
    void inputAsyn(std::shared_ptr<PACKAGE_INFO> & pkg);
    void handleResp(std::shared_ptr<PACKAGE_INFO> & pPkgInfo);

    SSL_CTX * getCtx(string & key);
    SSL_CTX * getDefaultCtx();
    std::shared_ptr<CSocketService> getService();
private:
    void buildThread();
    void threadHandleSocket();
    void threadHandleEvent();
    void threadHandleAsyn();


    void h2BuildBodyFrame(std::shared_ptr<PACKAGE_INFO> & pPkgInfo);
    void h2BuildHeadFrame(std::shared_ptr<PACKAGE_INFO> & pPkgInfo);
    void buildHeadResp(string & strInfo, std::shared_ptr<PACKAGE_INFO> & pPkgInfo); 
    bool getSndMsg(string & strMsg, string & strData, int pos);
    bool finishToSnd(std::shared_ptr<PACKAGE_INFO> & pPkgInfo);
    
private:
    std::atomic<bool>                   m_stop;
    std::shared_ptr<CSocketService>     m_sockService;
    std::shared_ptr<CAsynFile>          m_asynFile;
    std::list<std::thread>	            m_threadPool;
    std::shared_ptr<SafeQueue<std::shared_ptr<PACKAGE_INFO>>>         m_QDispatch;
    std::shared_ptr<SafeQueue<std::shared_ptr<PACKAGE_INFO>>>         m_QAsyn;
    

};
#define  CHandleMsgS  (CHandleMsg::getInstance())