#pragma once
#include "common.h"
#include "util.h"
#include "log.h"
#include "h2Frame.h"
#include "h2Session.h"
#include "socketService.h"
#include "h2pack.h"

class CH2Session {

public:
    CH2Session();
    ~CH2Session();
    void increasePeerWindowSize(std::uint32_t winSize);
    void setDependencyStream(uint32_t dependencyStream);
    uint32_t getDependencyStream();
    void setPriorityWeight(uint32_t weight);
    uint32_t getPriorityWeight();
    int hpackParse(HTTP2_HEADER_FIELD* field, int len);
    std::shared_ptr<hpack> getHPack();
    int GetPeerWindowSize();
    void setState(stream_state_e state);
    stream_state_e getState();
public:
    uint32_t        m_selfStream;
private:
    std::uint32_t   m_peerControlWindowSize;
    
    uint32_t        m_dependencyStream;
    uint32_t        m_priorityWeight;

    stream_state_e  m_streamState;

    std::shared_ptr<hpack>  m_hpack;
};