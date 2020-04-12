#pragma once
#include "h2Frame.h"
#include "h2Session.h"
#include "socketService.h"

class CH2Mng 
{
    typedef std::function<void(std::shared_ptr<SOCKINFO> & info)> CallbackFun;
    typedef struct _Command {
        int              commandID;
        CallbackFun      callbackFun;
    } Command;
    void initFun();
    void initHeaderTable();
public:
    CH2Mng();
    ~CH2Mng();
    void preface(std::shared_ptr<SOCKINFO> & info);
    int  http2RawData(std::shared_ptr<SOCKINFO> & info, string & strRawData);    
    void getStaticTable(map<int, pair<string, string> > & table);
    
private:
    bool checkPreface(std::shared_ptr<SOCKINFO> & info, string & strRawData);
    void makePackage(std::shared_ptr<SOCKINFO> & info);
    void makeSession(std::shared_ptr<SOCKINFO> & info, std::shared_ptr<CH2Session> & h2Session);
    std::shared_ptr<CH2Session> getSession(std::uint32_t streamInd);
    int getTableSize();
    void insertTable(int index, string & name, string & value);
    bool getTableInfo(int index, string & name, string & value);
    
    void h2Data(std::shared_ptr<SOCKINFO> & info);
    void h2Ping(std::shared_ptr<SOCKINFO> & info);
    void h2Header(std::shared_ptr<SOCKINFO> & info);
    void h2WindowUpdate(std::shared_ptr<SOCKINFO> & info);
    void h2Settings(std::shared_ptr<SOCKINFO> & info);

    void h2SendSettingAck(std::shared_ptr<SOCKINFO> & info);
    void h2SendGoaway(std::shared_ptr<SOCKINFO> & info, uint32_t last_stream_ind, uint32_t error_code);
    void h2EndHeader(std::shared_ptr<SOCKINFO> & info, std::shared_ptr<CH2Session> & h2Session);
private:
    std::map<int, Command>	 m_FunH2Mothed;
    std::map<uint32_t, std::shared_ptr<CH2Session>>     m_streamSession;

    map<int, pair<string, string> > m_header_static_table;

    map<int, pair<string, string> > m_header_table;
    std::mutex                      m_table_lock;


    std::uint32_t m_headerTableSize;
    std::uint32_t m_maxFrameSize;
    bool          m_enablePush;
    std::uint32_t m_maxConcurrentStreams;
    std::uint32_t m_initialPeerWindowSize;
    std::uint32_t m_initialLocalWindowSize;
    std::uint32_t m_peerControlWindowSize;
    std::uint32_t m_localControlWindowSize;
    std::uint32_t m_maxHeaderListSize;
};
