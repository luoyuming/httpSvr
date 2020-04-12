#pragma once
#include "common.h"
class CConvertSet
{
public:
    CConvertSet();
    ~CConvertSet();
     void UnicodeToUTF8(const std::wstring & wstr, std::string & str);
     void UTF8ToUnicode(const std::string & str, std::wstring & wstr);
private:

};


