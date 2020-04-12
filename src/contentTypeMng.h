#pragma once
#include "common.h"
#include "msgDef.h"


class ContentTypeMng {
public:
    ContentTypeMng(bool bCT=true);
    ~ContentTypeMng();
    bool findExtName(string & extName);
    string getContentType(string & extName);
private:
  
private:
    std::set<string>	                    m_gzExt;  
    std::map<string, string>                m_mpContentType;
    string                                  m_no_know_type;
};