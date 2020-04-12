#pragma once
#include "common.h"
#include "msgDef.h"
#include "util.h"
#include "sslParam.h"
#include "h2Frame.h"

enum class IOType
{
    IOUnknow = 0,
    IORead = 1,
    IOWrite,
    IOAccept,
    IOClose,
};


#ifdef WIN_OS 

struct SESSION_INFO;

typedef struct tagPER_IO_DATA
{
    OVERLAPPED Overlapped;
    WSABUF DataBuf;
    char buffer[DATA_BUFSIZE];
    std::shared_ptr<SESSION_INFO> session;
    IOType type;
} PER_IO_DATA, *LPPER_IO_DATA;


struct SESSION_INFO {
    SOCKET sockAccept;
    SOCKET sockListen;
    std::shared_ptr<PER_IO_DATA> sndIO;
    std::shared_ptr<PER_IO_DATA> recvIO;
    std::shared_ptr<CSslParam>  sslParam;
    string strData;
};
#endif


struct FORM_DATA_INFO {
    string fileName;
    string name;
    string data;
};


struct CHANNEL_INFO {
    PORT_TYPE           portType;    
    SSL		            *ssl_;
    SOCKET              sock;
    SSL_CTX				*sslCtx;
    bool				ssl_connect;
    
};

class CH2Mng;
class CH2Session;

struct PACKAGE_INFO {
    
#ifdef WIN_OS 
    std::shared_ptr<SESSION_INFO> session;
#endif
    std::shared_ptr<CH2Mng> h2Mng;
    std::shared_ptr<CH2Session> h2Session;
    bool bH2RespHead;

    int commandID;
    int bodyLen;
    string ip;
    string uri; //url+parameter
    string url; 
    int pos;   
    bool bFCGIResp;
    string strResp;  
    string strHead;
    string strCRLF; //\r\n
    string strFieldFlag; //field of http head to last one   "\r\n\r\n"	
    string strBody;
    string contentType;   //user to define  "Content-Type: text/html";
    string contentEncoding;
    std::map<string, string> mOriginalParam; //not to decode parameter
    std::map<string, string> mParam; //decode urlcode of head parameter x-www-form-urlencoded
    std::unordered_map<string, string> mField; //field of http head 
    std::unordered_map<string, string> mBodyUrlencoded; //x-www-form-urlencoded of body
    vector<std::shared_ptr<FORM_DATA_INFO> > vecFormData;
    std::shared_ptr<CHANNEL_INFO> channel;
    string fileNameAsyn;
    string curPath; //pwd path of runing program
    std::shared_ptr<SERVICE_CONFIG_INFO> serviceConfig;

    PACKAGE_INFO() {
        bH2RespHead = false;
    }
};



typedef struct _SOCKINFO
{
#ifdef WIN_OS 
    std::shared_ptr<SESSION_INFO> session;
#endif

    std::shared_ptr<CH2Mng> h2Mng;
    HTTP2_CONNECT_STATE     alpnState;
    std::shared_ptr<string> h2RawData;
    HTTP2_FRAME*            h2Head;


    int					count;
    string              ip;
    unsigned short      port;
    std::shared_ptr<UTIL_SELF::Timer> pTimer;
    string strChunkBody;
    int extLen;
    string extData;
    int pkgLen;
    std::shared_ptr<PACKAGE_INFO>           pPkgInfo;
    std::vector<std::shared_ptr<string> >   vecBody;
    std::shared_ptr<CHANNEL_INFO>           channel;
    unsigned int  pos;
    char data[DATA_BUFSIZE];
} SOCKINFO;


class CSocketService {

public:
    CSocketService();
    ~CSocketService();
    bool init(XML_SYSTEM_CONFIG && config);
    bool doEvent();    
    void stop();
    void notifySnd(std::shared_ptr<PACKAGE_INFO> & pPkgInfo, string & strSnd);
    void notifyRecv(std::shared_ptr<PACKAGE_INFO> & pPkgInfo);
    SSL_CTX * getCtx(string & key);
    void getMothedParam(std::shared_ptr<PACKAGE_INFO> & pInfo, string & strInfo);
private:
    bool notify(SOCKET sock, IOType type);
    bool initWSAfunc(SOCKET sock);
    int  setNonblocking(SOCKET sock);   
    bool createSocket(SOCKET & sock, int port);   
    int  getErrorNo(); 
#ifdef WIN_OS 
    void releaseIO(LPPER_IO_DATA perIoData);
    void releaseSession(std::shared_ptr<SESSION_INFO> & key);
    void postAccept(SOCKET sock);
    void doAccept(LPPER_IO_DATA perIoData, int len);
    void onRecv(LPPER_IO_DATA perIoData, int len);
    void postRecv(LPPER_IO_DATA perIoData);
    void onSnd(LPPER_IO_DATA perIoData, int len);
    void postSnd(LPPER_IO_DATA perIoData, string & strSnd);
    void prevRecv(LPPER_IO_DATA perIoData, int len);
    
#else 
    void doAccept(SOCKET sock, PORT_TYPE portType);
    void doRecv(SOCKET fd, PORT_TYPE portType, SSL * ssl);
    void postSnd(SOCKET fd);
    void postRecv(SOCKET fd);
    void doSnd(SOCKET fd, PORT_TYPE portType, SSL * ssl);
#endif
    
    PORT_TYPE getSockType(SOCKET fd);
    void closeConnect(SOCKET fd, bool Erase = true);
    void eraseSockInfo(SOCKET fd);
    void inputSockInfo(shared_ptr<SOCKINFO> & sockInfo);
    void initSockInfo(shared_ptr<SOCKINFO> & sockInfo, SOCKET fd, PORT_TYPE portType, string & strIP, int port);
    int  handleRawDataHttp(shared_ptr<SOCKINFO> & sockInfo, char *pMsg, int recvLen);
    int  handleDataHttp(shared_ptr<SOCKINFO> & sockInfo, char *pMsg, int recvLen);
    int  handleRawData(shared_ptr<SOCKINFO> & sockInfo, char *pMsg, int recvLen);
    char * headEnd(char *pMsg, string & strFlag, int & recvLen);
    void handleMothed(std::shared_ptr<PACKAGE_INFO> & pInfo);
    void handleField(std::shared_ptr<PACKAGE_INFO> & pInfo);
    
    void getUrlInfo(std::shared_ptr<PACKAGE_INFO> & pInfo, string && strInfo);
    void getLNFlag(string & flag, string & strInfo);
    bool fowardMsg(shared_ptr<SOCKINFO> & sockInfo, bool & bPost,bool & bChunked, int & len);
    int  countPackage(shared_ptr<SOCKINFO> & sockInfo);
    void handleTimeover();
    void eraseSndMsg(SOCKET fd);
    bool getSndPackage(SOCKET fd, std::shared_ptr<PACKAGE_INFO> & pPkgInfo);
    void splitInfo(vector<string> & vecInfo, string & strData);
    void doSplitCode(std::shared_ptr<PACKAGE_INFO> & pPkgInfo, string & strData);
    static int sslServernameCB(SSL *s, int *ad, void *arg);
    static int alpnSelectProtoCB(SSL *ssl, const uint8_t **out, uint8_t *outlen,
        const uint8_t *in, unsigned int inlen, void *arg);
    static int selectNextProtocol(uint8_t **out, uint8_t *outlen, const uint8_t *in,
        unsigned int inlen, const char *key, unsigned int keylen);

    bool initSSL();
    void sslAcept(SOCKET fd, shared_ptr<SOCKINFO> & info);
    void sslHandleRead(SOCKET fd);
    void sslHandleHandshake(SOCKET fd, shared_ptr<SOCKINFO> & info);
    void sslHandleDataRead(SOCKET fd, shared_ptr<SOCKINFO> & info);
    void sslHandleDataWrite(SOCKET fd, shared_ptr<SOCKINFO> & info);

    bool bioWrite(BIO *bio, string & strData);
    bool bioRead(BIO *bio, string & strData);
    bool sslWrite(SSL *ssl, string & strData);
    bool sslRead(SSL *ssl, string & strData, BIO *bio);
public:    
    void inputSndPackage(SOCKET fd, std::shared_ptr<PACKAGE_INFO> & pPkgInfo);
    bool chunkedMsg(shared_ptr<SOCKINFO> & sockInfo, bool & bChunked, int & retLen);
    bool existCache(std::shared_ptr<PACKAGE_INFO> & pPkgInfo);
    void sslCode(std::shared_ptr<PACKAGE_INFO> & pPkgInfo);

    SSL_CTX    *g_sslTemp;
private:
    std::map<string, int>				m_mpMothed;
    std::list<string>				    m_liLN;
    IP_TYPE                             m_ipType;


    HANDLE              m_handlePort;
    SOCKET              m_listenSocketTcp;
    SOCKET              m_listenSocketHttp;
    SOCKET              m_listenSocketSSL;

    std::map<SOCKET, shared_ptr<SOCKINFO>>          m_mpSockInfo;  
    std::map<string, SSL_CTX*>                      m_mpCtxSSL;
    
#ifdef WIN_OS 
    std::mutex									    m_sslLock;

    LPFN_ACCEPTEX                                   m_acceptEx; 
    LPFN_GETACCEPTEXSOCKADDRS                       m_getAcceptExSockaddrs;
    std::mutex                                      m_lock;   
    std::list<std::shared_ptr<SESSION_INFO>>        m_liSession;
#endif

    std::mutex									    m_sndLock;
    std::map<SOCKET, shared_ptr<PACKAGE_INFO>>		m_sndMsg;
};


