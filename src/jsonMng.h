#pragma once
#include "common.h"
#include "handleJson.h"

static const string  JSON_FILE_SYSTEM = "service.json";

class CJsonMng {

public:
    bool init(string && path);
    std::shared_ptr<SERVICE_CONFIG_INFO> getServiceConfig(string && strDomain);
    void getAllConfig(vector<std::shared_ptr<SERVICE_CONFIG_INFO>> & vecConfig);
private:
   
private:
    std::mutex                  m_lock;
    JSON_CONFIG_INFO            m_jsonConfig;
    string                      m_defaultDomain;
    std::map<string, std::shared_ptr<SERVICE_CONFIG_INFO>>       m_mpDomainInfo; //domain-->index
};