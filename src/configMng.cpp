#include "configMng.h"


CMngConfig::CMngConfig()
{
    m_xmlInfo = std::make_shared<CHandleXml>();
    m_jsonInfo = std::make_shared<CJsonMng>();
}

void CMngConfig::Quit()
{

}

bool CMngConfig::init()
{  
    if (!m_xmlInfo->init()) {
        return false;
    }
    if (!m_jsonInfo->init(m_xmlInfo->getPath())) {
        return false;
    }
  
    return true;
}

std::shared_ptr<CHandleXml> CMngConfig::getXmlPtr()
{
    return m_xmlInfo;
}

std::shared_ptr<CJsonMng>  CMngConfig::getJsonPtr()
{
    return m_jsonInfo;
}