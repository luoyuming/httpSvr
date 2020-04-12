#include "h2Session.h"

CH2Session::CH2Session() :m_peerControlWindowSize(HTTP2_DEFAULT_WINDOW_SIZE)
, m_dependencyStream(0), m_priorityWeight(16)
, m_streamState(stream_state_e::stream_idle)

{
    m_hpack = std::make_shared<hpack>();
}

CH2Session::~CH2Session()
{

}

stream_state_e CH2Session::getState()
{
    return m_streamState;
}

void CH2Session::setState(stream_state_e state)
{
    m_streamState = state;
}

int CH2Session::GetPeerWindowSize()
{
    return m_peerControlWindowSize;
}

void CH2Session::increasePeerWindowSize(std::uint32_t winSize)
{
    m_peerControlWindowSize += winSize;
}

uint32_t CH2Session::getDependencyStream()
{
    return m_dependencyStream;
}

void CH2Session::setDependencyStream(uint32_t dependencyStream)
{   
    m_dependencyStream = dependencyStream;
    return;
}

void CH2Session::setPriorityWeight(uint32_t weight)
{
    m_priorityWeight = weight;
}

uint32_t CH2Session::getPriorityWeight()
{
    return m_priorityWeight;
}

int CH2Session::hpackParse(HTTP2_HEADER_FIELD* field, int len)
{   
    return m_hpack->parse(field, len);
}

std::shared_ptr<hpack> CH2Session::getHPack()
{
    return m_hpack;
}