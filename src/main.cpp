
#include "common.h"
#include "log.h"

#include "handleMsg.h"

void SignalHandler(int sig);

int main(int argc, char** argv)
{ 
 

#ifdef  LINUX_OS 
    signal(SIGABRT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGSEGV, SIG_IGN);
    signal(SIGTRAP, SIG_IGN);
    signal(SIGUSR1, SignalHandler);
    signal(SIGTERM, SignalHandler);
    signal(SIGINT, SignalHandler);
    signal(SIGHUP, SignalHandler);
    signal(SIGCHLD, SignalHandler);
    signal(SIGQUIT, SignalHandler);
#endif

    LOG_INIT(argv[0]);
    LOG_INFO("begin to run...%s", argv[0]);   
   
    if (!CHandleMsgS->init()) {
        LOG_ERROR("quit to run...%s", argv[0]);
        return -1;
    }
    CHandleMsgS->starSvr();   
    LOG_INFO("httpSvr service is runing...");
    CHandleMsgS->join();
    LOG_INFO("quit to run...%s", argv[0]);  
    return 0;
}

void SignalHandler(int sig)
{
#ifdef  LINUX_OS 
    LOG_INFO("server received a signal: %d", sig);
    switch (sig)
    {
    case SIGTERM:
    case SIGINT:
    case SIGKILL:
    case SIGQUIT:
    case SIGHUP: {
        LOG_INFO("quit signal: %d", sig);
        CHandleMsgS->stop();      
    }
        break;
    case SIGALRM:
        break;
    case SIGUSR1:
        break;
    default:
        break;
    }
#endif
}

