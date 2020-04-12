#include "h2Mng.h"
#include "handleMsg.h"
#include "log.h"

static const char* error_table[] = {
    "NO_ERROR",
    "PROTOCOL_ERROR",
    "INTERNAL_ERROR",
    "FLOW_CONTROL_ERROR",
    "SETTINGS_TIMEOUT",
    "STREAM_CLOSED",
    "FRAME_SIZE_ERROR",
    "REFUSED_STREAM",
    "CANCEL",
    "COMPRESSION_ERROR",
    "CONNECT_ERROR",
    "ENHANCE_YOUR_CALM",
    "INADEQUATE_SECURITY",
    "HTTP_1_1_REQUIRED"
};


CH2Mng::CH2Mng() :m_headerTableSize(4096)
, m_maxFrameSize(16384)
, m_enablePush(true)
, m_maxConcurrentStreams(101)
, m_initialPeerWindowSize(HTTP2_DEFAULT_WINDOW_SIZE)
, m_initialLocalWindowSize(HTTP2_DEFAULT_WINDOW_SIZE)
, m_peerControlWindowSize(HTTP2_DEFAULT_WINDOW_SIZE)
, m_localControlWindowSize(HTTP2_DEFAULT_WINDOW_SIZE)
, m_maxHeaderListSize(0xFFFFFFFF)
{
    initFun();
    initHeaderTable();
}

CH2Mng::~CH2Mng()
{

}

void CH2Mng::initFun()
{
    std::uint32_t i = 0;
    Command h2Mothed[] = {
        { HTTP2_FRAME_TYPE_SETTINGS, std::bind(&CH2Mng::h2Settings, this, _1) },
        { HTTP2_FRAME_TYPE_WINDOW_UPDATE, std::bind(&CH2Mng::h2WindowUpdate, this, _1) },
        { HTTP2_FRAME_TYPE_HEADERS, std::bind(&CH2Mng::h2Header, this, _1) },
        { HTTP2_FRAME_TYPE_PING, std::bind(&CH2Mng::h2Ping, this, _1) },
        { HTTP2_FRAME_TYPE_DATA, std::bind(&CH2Mng::h2Data, this, _1) },
    };

    for (i = 0; i < sizeof(h2Mothed) / sizeof(h2Mothed[0]); ++i)
    {
        m_FunH2Mothed[h2Mothed[i].commandID] = h2Mothed[i];
    }
}

void CH2Mng::initHeaderTable()
{
    std::lock_guard<std::mutex>lck(m_table_lock);
    m_header_table[1] = make_pair(string(":authority"), string(""));
    m_header_table[2] = make_pair(string(":method"), string("GET"));
    m_header_table[3] = make_pair(string(":method"), string("POST"));
    m_header_table[4] = make_pair(string(":path"), string("/"));
    m_header_table[5] = make_pair(string(":path"), string("/index.html"));
    m_header_table[6] = make_pair(string(":scheme"), string("http"));
    m_header_table[7] = make_pair(string(":scheme"), string("https"));
    m_header_table[8] = make_pair(string(":status"), string("200"));
    m_header_table[9] = make_pair(string(":status"), string("204"));
    m_header_table[10] = make_pair(string(":status"), string("206"));
    m_header_table[11] = make_pair(string(":status"), string("304"));
    m_header_table[12] = make_pair(string(":status"), string("400"));
    m_header_table[13] = make_pair(string(":status"), string("404"));
    m_header_table[14] = make_pair(string(":status"), string("500"));
    m_header_table[15] = make_pair(string("accept-charset"), string(""));
    m_header_table[16] = make_pair(string("accept-encoding"), string("gzip, deflate"));
    m_header_table[17] = make_pair(string("accept-language"), string(""));
    m_header_table[18] = make_pair(string("accept-ranges"), string(""));
    m_header_table[19] = make_pair(string("accept"), string(""));
    m_header_table[20] = make_pair(string("access-control-allow-origin"), string(""));
    m_header_table[21] = make_pair(string("age"), string(""));
    m_header_table[22] = make_pair(string("allow"), string(""));
    m_header_table[23] = make_pair(string("authorization"), string(""));
    m_header_table[24] = make_pair(string("cache-control"), string(""));
    m_header_table[25] = make_pair(string("content-disposition"), string(""));
    m_header_table[26] = make_pair(string("content-encoding"), string(""));
    m_header_table[27] = make_pair(string("content-language"), string(""));
    m_header_table[28] = make_pair(string("content-length"), string(""));
    m_header_table[29] = make_pair(string("content-location"), string(""));
    m_header_table[30] = make_pair(string("content-range"), string(""));
    m_header_table[31] = make_pair(string("content-type"), string(""));
    m_header_table[32] = make_pair(string("cookie"), string(""));
    m_header_table[33] = make_pair(string("date"), string(""));
    m_header_table[34] = make_pair(string("etag"), string(""));
    m_header_table[35] = make_pair(string("expect"), string(""));
    m_header_table[36] = make_pair(string("expires"), string(""));
    m_header_table[37] = make_pair(string("from"), string(""));
    m_header_table[38] = make_pair(string("host"), string(""));
    m_header_table[39] = make_pair(string("if-match"), string(""));
    m_header_table[40] = make_pair(string("if-modified-since"), string(""));
    m_header_table[41] = make_pair(string("if-none-match"), string(""));
    m_header_table[42] = make_pair(string("if-range"), string(""));
    m_header_table[43] = make_pair(string("if-unmodified-since"), string(""));
    m_header_table[44] = make_pair(string("last-modified"), string(""));
    m_header_table[45] = make_pair(string("link"), string(""));
    m_header_table[46] = make_pair(string("location"), string(""));
    m_header_table[47] = make_pair(string("max-forwards"), string(""));
    m_header_table[48] = make_pair(string("proxy-authenticate"), string(""));
    m_header_table[49] = make_pair(string("proxy-authorization"), string(""));
    m_header_table[50] = make_pair(string("range"), string(""));
    m_header_table[51] = make_pair(string("referer"), string(""));
    m_header_table[52] = make_pair(string("refresh"), string(""));
    m_header_table[53] = make_pair(string("retry-after"), string(""));
    m_header_table[54] = make_pair(string("server"), string(""));
    m_header_table[55] = make_pair(string("set-cookie"), string(""));
    m_header_table[56] = make_pair(string("strict-transport-security"), string(""));
    m_header_table[57] = make_pair(string("transfer-encoding"), string(""));
    m_header_table[58] = make_pair(string("user-agent"), string(""));
    m_header_table[59] = make_pair(string("vary"), string(""));
    m_header_table[60] = make_pair(string("via"), string(""));
    m_header_table[61] = make_pair(string("www-authenticate"), string(""));

    m_header_static_table = m_header_table;
}

void CH2Mng::preface(std::shared_ptr<SOCKINFO> & info)
{
   auto pPkgInfo = std::make_shared<PACKAGE_INFO>();
#ifdef WIN_OS 
   pPkgInfo->session = info->session;
#endif
   pPkgInfo->pos = 0;
   pPkgInfo->channel = info->channel;
   pPkgInfo->strResp = HTTP2_PERFACE;
   CHandleMsgS->notifyMsg(pPkgInfo);

   //LOG_INFO("%s", pPkgInfo->strResp.c_str());
   return;
}

void CH2Mng::makeSession(std::shared_ptr<SOCKINFO> & info, std::shared_ptr<CH2Session> & h2Session)
{   
    info->pPkgInfo = std::make_shared<PACKAGE_INFO>();    
    info->pPkgInfo->channel = info->channel;
#ifdef WIN_OS 
    info->pPkgInfo->session = info->session;
#endif
    info->pPkgInfo->h2Mng = info->h2Mng;
    info->pPkgInfo->h2Session = h2Session;
}

void CH2Mng::makePackage(std::shared_ptr<SOCKINFO> & info)
{
    info->pPkgInfo = std::make_shared<PACKAGE_INFO>();
    info->pPkgInfo->channel = info->channel;
#ifdef WIN_OS 
    info->pPkgInfo->session = info->session;
#endif

}

bool CH2Mng::checkPreface(std::shared_ptr<SOCKINFO> & info, string & strRawData)
{
    bool bResult = false;
    if ((HTTP2_CONNECT_STATE::HTTP2_ALPN_PREFACE == info->alpnState)
        || (HTTP2_CONNECT_STATE::HTTP2_ALPN_NONE == info->alpnState)
        ) {
        std::uint32_t len = static_cast<int>(strRawData.size());
        if (len >= HTTP2_PREFACE_LEN) {
            string strPreface(&strRawData[0], HTTP2_PREFACE_LEN);
            if (HTTP2_PERFACE == strPreface) {
                //LOG_INFO("(%s) alpnState=(%d)", strPreface.c_str(), info->alpnState);
                if (HTTP2_CONNECT_STATE::HTTP2_ALPN_NONE == info->alpnState) {
                    preface(info);
                }
                info->alpnState = HTTP2_CONNECT_STATE::HTTP2_ALPN_CONNECT;
                bResult = true;
                strRawData.erase(0, HTTP2_PREFACE_LEN);
            } //if (HTTP2_PERFACE
        }// if (len
    }
    return bResult;
}

int  CH2Mng::http2RawData(std::shared_ptr<SOCKINFO> & info, string & strRawData)
{
    string & rawData = *(info->h2RawData);
    rawData += strRawData;
    if (HTTP2_CONNECT_STATE::HTTP2_ALPN_CONNECT != info->alpnState) {
        if (!checkPreface(info, rawData)) {
            return 0;
        }
    }

    //LOG_INFO("(%d) %s", rawData.size(), UTIL_SELF::strToHex(rawData).c_str());

    std::uint32_t size = static_cast<std::uint32_t>(rawData.size());
    if (size >= HTTP2_FRAME_HEAD_LEN) {       
        info->h2Head = reinterpret_cast<HTTP2_FRAME*>(&rawData[0]);
        std::uint8_t type = rawData[3];
        std::uint32_t len = info->h2Head->length.len24;       
        std::uint32_t payloadLen = ::ntohl(len << 8) & 0x00FFFFFFU;
        LOG_INFO("%d payload_len %d recv %d", type, payloadLen, size);
        len = (HTTP2_FRAME_HEAD_LEN + payloadLen);
        if (size >= payloadLen) {
            auto it = m_FunH2Mothed.find(type);
            if (it != m_FunH2Mothed.end()) {                
                auto & cmd = it->second;
                cmd.callbackFun(info);
            }
            rawData.erase(0, len);
        }
    }
    return 0;
}

void CH2Mng::h2Ping(std::shared_ptr<SOCKINFO> & info)
{
    std::uint32_t streamInd = info->h2Head->identifier;
    streamInd = ntohl(streamInd << 1) & 0x7FFFFFFFU;
    LOG_INFO("h2Ping streamInd %d", streamInd);

    makePackage(info);

    string & strResp = info->pPkgInfo->strResp;
    strResp.resize(HTTP2_FRAME_HEAD_LEN + sizeof(HTTP2_FRAME_PING));
    HTTP2_FRAME* ping_ack = (HTTP2_FRAME*)&strResp[0];

    ping_ack->length.len3b[0] = 0x00;
    ping_ack->length.len3b[1] = 0x00;
    ping_ack->length.len3b[2] = sizeof(HTTP2_FRAME_PING); //length is 8
    ping_ack->type = HTTP2_FRAME_TYPE_PING;
    ping_ack->flags = HTTP2_FRAME_FLAG_PING_ACK;
    ping_ack->r = HTTP2_FRAME_R_UNSET;
    ping_ack->identifier = info->h2Head->identifier;

    HTTP2_FRAME_PING* ping_ack_frm = (HTTP2_FRAME_PING*)((char*)ping_ack + sizeof(HTTP2_FRAME));
    HTTP2_FRAME_PING* ping_frm = (HTTP2_FRAME_PING*)(ping_ack->payload);
    memcpy(ping_ack_frm->data, ping_frm->data, sizeof(HTTP2_FRAME_PING));

    CHandleMsgS->handleResp(info->pPkgInfo);
}

void CH2Mng::h2Header(std::shared_ptr<SOCKINFO> & info)
{
    std::uint32_t streamInd = info->h2Head->identifier;
    streamInd = ntohl(streamInd << 1) & 0x7FFFFFFFU;
    LOG_INFO("h2Header streamInd %d", streamInd);

    std::uint32_t padding_len = 0;
    std::uint32_t dep_ind = 0;
    std::uint8_t weight;
    int offset = 0;
    if ((info->h2Head->flags & HTTP2_FRAME_FLAG_PADDED) == HTTP2_FRAME_FLAG_PADDED) {
        LOG_INFO(">> PAD");
        HTTP2_FRAME_HEADER_PAD * header1 = (HTTP2_FRAME_HEADER_PAD*)(info->h2Head->payload);
        padding_len = header1->pad_length;
        offset += sizeof(HTTP2_FRAME_HEADER_PAD);
    }
    if ((info->h2Head->flags & HTTP2_FRAME_FLAG_PRIORITY) == HTTP2_FRAME_FLAG_PRIORITY) {

        HTTP2_FRAME_HEADER_WEIGHT * header_weight = (HTTP2_FRAME_HEADER_WEIGHT*)(info->h2Head->payload + offset);
        dep_ind = ntohl(header_weight->dependency << 1) & 0x7FFFFFFFU;
        weight = header_weight->weight;
        offset += sizeof(HTTP2_FRAME_HEADER_WEIGHT);

        auto h2Session = getSession(streamInd);
        h2Session->setDependencyStream(streamInd);
        h2Session->setPriorityWeight(weight);
        if (header_weight->e == HTTP2_STREAM_DEPENDENCY_E_SET && dep_ind != 0) {
            for (auto & item : m_streamSession) {
                if ((item.second->getDependencyStream() == dep_ind) && (item.second->m_selfStream != dep_ind)) {
                    item.second->setDependencyStream(streamInd);
                }
            }
            LOG_INFO(">> PRIORITY <<, Weight: %d, e: %d, depends on stream %u", weight, header_weight->e, dep_ind);
        }

    }
    uint32_t payload_len = info->h2Head->length.len24;
    payload_len = ntohl(payload_len << 8) & 0x00FFFFFFU;
    int fragment_len = payload_len - offset - padding_len;
    HTTP2_FRAME_HEADER_FRAGMENT * header3 = (HTTP2_FRAME_HEADER_FRAGMENT*)(info->h2Head->payload + offset);
    auto h2Session = getSession(streamInd);
    if (h2Session->hpackParse((HTTP2_HEADER_FIELD*)header3->block_fragment, fragment_len) < 0) {

        h2SendGoaway(info, streamInd, HTTP2_COMPRESSION_ERROR);
        return;
    }

    if ((info->h2Head->flags & HTTP2_FRAME_FLAG_END_HEADERS) == HTTP2_FRAME_FLAG_END_HEADERS) {
        h2EndHeader(info, h2Session);
        if (HTTP_GET == info->pPkgInfo->commandID) {
            if (info->pPkgInfo->url.empty()) {
                LOG_ERROR("error to get url");
                h2SendGoaway(info, streamInd, HTTP2_PROTOCOL_ERROR);
                return;
            }
            else {
                LOG_INFO("get %s", info->pPkgInfo->url.c_str());
                CHandleMsgS->inputMsg(info->pPkgInfo);
            }
        }
        h2Session.reset();
    }

    return;
}



int CH2Mng::getTableSize()
{
    std::lock_guard<std::mutex>lck(m_table_lock);
    int size = static_cast<int>(m_header_table.size());
    return size;
}

void CH2Mng::insertTable(int index, string & name, string & value)
{
    std::lock_guard<std::mutex>lck(m_table_lock);
    m_header_table[index] = make_pair(name, value);
}


void CH2Mng::h2SendGoaway(std::shared_ptr<SOCKINFO> & info, uint32_t last_stream_ind, uint32_t error_code)
{

    LOG_INFO("Send GOAWAY for %d as %s", last_stream_ind, error_table[error_code]);
    makePackage(info);
    int len = HTTP2_FRAME_HEAD_LEN + sizeof(HTTP2_FRAME_GOAWAY);
    string & strResp = info->pPkgInfo->strResp;
    strResp.clear();
    strResp.resize(len);

    HTTP2_FRAME* head = (HTTP2_FRAME*)&strResp[0];
    //HTTP2_Frame go_away;
    head->length.len3b[0] = 0x00;
    head->length.len3b[1] = 0x00;
    head->length.len3b[2] = sizeof(HTTP2_FRAME_GOAWAY);
    head->type = HTTP2_FRAME_TYPE_GOAWAY;
    head->flags = HTTP2_FRAME_FLAG_UNSET;
    head->r = HTTP2_FRAME_R_UNSET;
    head->identifier = 0;

    HTTP2_FRAME_GOAWAY* go_away = (HTTP2_FRAME_GOAWAY*)(&strResp[HTTP2_FRAME_HEAD_LEN]);
    go_away->r = 0;
    go_away->last_stream_id = htonl(last_stream_ind) >> 1;
    go_away->error_code = htonl(error_code);

    CHandleMsgS->handleResp(info->pPkgInfo);
    return;
}

std::shared_ptr<CH2Session> CH2Mng::getSession(std::uint32_t streamInd)
{
    auto it = m_streamSession.find(streamInd);
    if (m_streamSession.end() == it) {
        auto session = std::make_shared<CH2Session>();
        session->m_selfStream = streamInd;
        m_streamSession[streamInd] = session;
        return session;
    }
    return it->second;
}

void CH2Mng::h2WindowUpdate(std::shared_ptr<SOCKINFO> & info)
{  
    HTTP2_FRAME_WINDOW_UPDATE* winUpdate = (HTTP2_FRAME_WINDOW_UPDATE*)(info->h2Head->payload);
    std::uint32_t win_size = ntohl(winUpdate->win_size << 1) & 0x7FFFFFFFU;
    LOG_INFO("h2WindowUpdate win_size=%d", win_size);
    if (win_size == 0) {

        return;
    }
    std::uint32_t streamInd = info->h2Head->identifier;
    streamInd = ntohl(streamInd << 1) & 0x7FFFFFFFU;
    LOG_INFO("h2WindowUpdate streamInd %d", streamInd);
    if (0 == streamInd) {
        m_peerControlWindowSize += win_size;
        printf("Current Peer Connection Window Size: %u %d\n", m_peerControlWindowSize, win_size);
    }
    else {
        getSession(streamInd)->increasePeerWindowSize(win_size);
    }
    return;
}

void CH2Mng::h2Settings(std::shared_ptr<SOCKINFO> & info)
{
    LOG_INFO("h2Settings");
    if ((info->h2Head->flags & HTTP2_FRAME_FLAG_SETTING_ACK) == HTTP2_FRAME_FLAG_SETTING_ACK) {
        return;
    }

    HTTP2_SETTINGS* clientSetting = (HTTP2_SETTINGS*)(info->h2Head->payload);
    std::uint16_t identifier = ::ntohs(clientSetting->identifier);
    std::uint32_t value = ntohl(clientSetting->value);
    switch (identifier) {
    case HTTP2_SETTINGS_HEADER_TABLE_SIZE:
        m_headerTableSize = value;
        LOG_INFO("  Recieved HTTP2_SETTINGS_HEADER_TABLE_SIZE %d\n", value);
        break;
    case HTTP2_SETTINGS_ENABLE_PUSH:
        m_enablePush = (value == 0 ? false : true);
        LOG_INFO("  Recieved HTTP2_SETTINGS_ENABLE_PUSH %d\n", value);
        break;
    case HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
        m_maxConcurrentStreams = value;
        LOG_INFO("  Recieved HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS %d\n", value);
        break;
    case HTTP2_SETTINGS_INITIAL_WINDOW_SIZE:
        m_initialPeerWindowSize = value;
        LOG_INFO("  Recieved HTTP2_SETTINGS_INITIAL_WINDOW_SIZE %d\n", value);
        break;
    case HTTP2_SETTINGS_MAX_FRAME_SIZE:
        m_maxFrameSize = value;
        LOG_INFO("  Recieved HTTP2_SETTINGS_MAX_FRAME_SIZE %d\n", value);
        break;
    case HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE:
        m_maxHeaderListSize = value;
        LOG_INFO("  Recieved HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE %d\n", value);
        break;
    default:
        break;
    }
    h2SendSettingAck(info);
    return;

}

void CH2Mng::h2SendSettingAck(std::shared_ptr<SOCKINFO> & info)
{
    std::uint32_t streamInd = info->h2Head->identifier;
    streamInd = ntohl(streamInd << 1) & 0x7FFFFFFFU;
    LOG_INFO("h2SendSettingAck streamInd %d", streamInd);
    makePackage(info);
    string & strResp = info->pPkgInfo->strResp;
    strResp.clear();
    strResp.resize(HTTP2_FRAME_HEAD_LEN);
    HTTP2_FRAME* settingAck = (HTTP2_FRAME*)&strResp[0];

    settingAck->length.len3b[0] = 0x00;
    settingAck->length.len3b[1] = 0x00;
    settingAck->length.len3b[2] = 0x00; //length is 0
    settingAck->type = HTTP2_FRAME_TYPE_SETTINGS;
    settingAck->flags = HTTP2_FRAME_FLAG_SETTING_ACK;
    settingAck->r = HTTP2_FRAME_R_UNSET;
    settingAck->identifier = htonl(streamInd) >> 1;

    CHandleMsgS->handleResp(info->pPkgInfo);
    return;
}


bool CH2Mng::getTableInfo(int index, string & name, string & value)
{  
    bool bResult = false;
    std::lock_guard<std::mutex>lck(m_table_lock);
    auto it = m_header_table.find(index);
    if (m_header_table.end() != it) {
        bResult = true;
        name = it->second.first;
        value = it->second.second;       
    }
    return bResult;
}

void CH2Mng::h2EndHeader(std::shared_ptr<SOCKINFO> & info, std::shared_ptr<CH2Session> & h2Session)
{ 
    makeSession(info, h2Session);
    auto & mpField = info->pPkgInfo->mField;
    string cookiesValue;

    int stable_size = getTableSize();
    int i = 0;
    std::shared_ptr<HTTP2_HEADER_INDEX> headerIndex;
    auto hpack = h2Session->getHPack();
    for (;;) {
        i = hpack->getNextIndex(headerIndex, i);
        if (i < 0) {
            break;
        }
        auto & item = *headerIndex;
        if ((item.index_type == index_type_e::type_with_indexing_indexed_name)
            || (item.index_type == index_type_e::type_with_indexing_new_name)
            )
        {
            string name, value;
            if (getTableInfo(item.index, name, value)) {
                stable_size += 1;
                insertTable(stable_size, name, item.value);                
            }
            else if ((!item.name.empty()) && (!item.value.empty())) {
                stable_size += 1;
                insertTable(stable_size, item.name, item.value);               
            }
        }

        if ((2 == item.index) && (index_type_e::type_indexed == item.index_type)) {
            info->pPkgInfo->commandID = HTTP_GET;
            LOG_INFO("commandID : get");
        }
        if ((3 == item.index) && (index_type_e::type_indexed == item.index_type)) {
            info->pPkgInfo->commandID = HTTP_POST;
            LOG_INFO("commandID : post");
        }
        if ((4 == item.index) || (5 == item.index)) {
            info->pPkgInfo->url = "/";
            LOG_INFO("url : /");
        }
        else {
            if (32 == item.index) {
                if (cookiesValue.empty()) {
                    cookiesValue = item.value.c_str();
                }
                else {
                    cookiesValue += ";";
                    cookiesValue += item.value.c_str();
                }
            }
            else {
                string name, value;
                getTableInfo(item.index, name, value);
                if (name.empty()) {
                    name = item.name;
                }
                if (value.empty()) {
                    value = item.value;
                }
                mpField[name.c_str()] = value.c_str();               
            }
        } 

        LOG_INFO("%d %s %s", item.index, item.name.c_str(), item.value.c_str());
    }

    if (!cookiesValue.empty()) {
        mpField["cookie"] = cookiesValue.c_str();
    }

    if (info->pPkgInfo->url.empty()) {     
        string key = ":path";
        auto it = mpField.find(key);
        if (mpField.end() != it) {           
            string strPath = it->second;          
            CHandleMsgS->getService()->getMothedParam(info->pPkgInfo, strPath);
        }
        else { 
            LOG_ERROR("not to find the path..");
        }
    }    
    return;
}

void CH2Mng::getStaticTable(map<int, pair<string, string> > & table)
{
    std::lock_guard<std::mutex>lck(m_table_lock);
    table = m_header_static_table;
}


void CH2Mng::h2Data(std::shared_ptr<SOCKINFO> & info)
{
    std::uint32_t streamInd = info->h2Head->identifier;
    streamInd = ntohl(streamInd << 1) & 0x7FFFFFFFU;
    LOG_INFO("h2Data streamInd %d", streamInd);
    auto h2Session = getSession(streamInd);

    string strBody;
    HTTP2_FRAME & frame_hdr = *(info->h2Head);
    int offset = 0;
    std::uint32_t padding_len = 0;
    if ((frame_hdr.flags & HTTP2_FRAME_FLAG_PADDED) == HTTP2_FRAME_FLAG_PADDED) {
        HTTP2_FRAME_DATA1 * data1 = (HTTP2_FRAME_DATA1*)(frame_hdr.payload);
        padding_len = data1->pad_length;
        offset += sizeof(HTTP2_FRAME_DATA1);
        strBody.append(data1->data_padding, frame_hdr.length.len24 - padding_len);
    }
    else {
        HTTP2_FRAME_DATA2 * data2 = (HTTP2_FRAME_DATA2*)(frame_hdr.payload);
        offset += sizeof(HTTP2_FRAME_DATA2);
        strBody.append(data2->data, frame_hdr.length.len24);
    }
    if ((frame_hdr.flags & HTTP2_FRAME_FLAG_END_STREAM) == HTTP2_FRAME_FLAG_END_STREAM) {
        if (streamInd == 0 || h2Session->getState() != stream_state_e::stream_half_closed_local)
            h2Session->setState(stream_state_e::stream_half_closed_remote);
        else 
            h2Session->setState(stream_state_e::stream_closed);
    }
    return;
}
