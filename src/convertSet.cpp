#include "convertSet.h"


CConvertSet::CConvertSet(){

}
CConvertSet::~CConvertSet() {

}

void CConvertSet::UnicodeToUTF8(const std::wstring & wstr, std::string & str)
{
 
    if (wstr.empty())
        return;
    std::size_t len = wstr.size();
    string strTemp;
    strTemp.resize(len*4);
    std::size_t ret = wcstombs(&strTemp[0], &wstr[0], len);
    if (ret > 0) {
        str = strTemp.substr(0, ret);
    }
    return;
}

void CConvertSet::UTF8ToUnicode(const std::string & str, std::wstring & wstr)
{
    if (str.empty())
        return;

    std::size_t len = str.size();
    wstring strTemp;
    strTemp.resize(len * 4);
    std::size_t ret = mbstowcs(&strTemp[0], &str[0], len);
    if (ret > 0) {
        wstr = strTemp.substr(0, ret);
    }
    return;
}


