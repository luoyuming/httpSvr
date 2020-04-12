/*
*  12797202@qq.com   name: luoyuming mobile 13925236752
*
* Copyright (c) 2020 www.dswd.net
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
* LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
* OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
* WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "jsonMng.h"
#include "log.h"
#include "util.h"

bool CJsonMng::init(string  && path)
{
    string strFilename = path + JSON_FILE_SYSTEM;  
    
    string strJson;   
    if (!UTIL_SELF::readFile(strFilename, strJson)) {
        LOG_ERROR("fault to read file %s", strFilename.c_str());
        return false;
    }
   
    auto handleJson(std::make_unique<CHandleJson>());
    if (!handleJson->extractJsonConfig(m_jsonConfig, strJson)) {
        LOG_ERROR("fault to parse json file %s", strFilename.c_str());
        return false;
    }
   
    bool bFirst = true;
    for (auto & server : m_jsonConfig.server) {
        auto config = std::make_shared<SERVICE_CONFIG_INFO>();
        *config = server;
        m_mpDomainInfo[server.server_name] = config;
        if (bFirst) {
            m_defaultDomain = server.server_name;
            bFirst = false;
        }
    }
    m_jsonConfig.server.clear();
    return true;
}

std::shared_ptr<SERVICE_CONFIG_INFO> CJsonMng::getServiceConfig(string && strDomain)
{
    std::shared_ptr<SERVICE_CONFIG_INFO> config;
    std::lock_guard<std::mutex>lck(m_lock);
    auto it = m_mpDomainInfo.find(strDomain);
    if (m_mpDomainInfo.end() != it) {
        config = it->second;
    }
    else {
        config = m_mpDomainInfo[m_defaultDomain];
    }
    return config;
}

void CJsonMng::getAllConfig(vector<std::shared_ptr<SERVICE_CONFIG_INFO>> & vecConfig)
{
    std::lock_guard<std::mutex>lck(m_lock);
    for (auto & item : m_mpDomainInfo) {
        vecConfig.push_back(item.second);
    }
}
