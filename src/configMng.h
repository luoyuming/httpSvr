#pragma once
#include "singleton.h"
#include "common.h"
#include "util.h"
#include "log.h"
#include "handleXml.h"
#include "jsonMng.h"


class CMngConfig : public SingletionEX<CMngConfig>
{
    SINGLETON_INIT_EX(CMngConfig);
    CMngConfig();
public:
    bool init();
    std::shared_ptr<CHandleXml> getXmlPtr();
    std::shared_ptr<CJsonMng>  getJsonPtr();
private:
    std::shared_ptr<CHandleXml> m_xmlInfo;
    std::shared_ptr<CJsonMng> m_jsonInfo;

};
#define  CMngConfigS  (CMngConfig::getInstance())