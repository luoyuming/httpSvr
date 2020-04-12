#include "util.h"
#include "convertSet.h"



namespace UTIL_SELF {

    std::string strToHex(const std::string str, std::string separator)
    {
        const std::string hex = "0123456789ABCDEF";
        std::stringstream ss;

        for (std::string::size_type i = 0; i < str.size(); ++i)
            ss << hex[(unsigned char)str[i] >> 4] << hex[(unsigned char)str[i] & 0xf] << separator;

        return ss.str();
    }

    string getPwd()
    {
        string strPath;
#ifdef  LINUX_OS        
        char  path1[1024] = { 0 };
        if (nullptr == getcwd(path1, sizeof(path1)))
        {
            strcpy(path1, "/");
        }
        else
        {
            strcat(path1, "/");
        }
        strPath = path1;
#endif
#ifdef  WIN_OS

        CConvertSet Convert;
        const int DATA_MAX_LEN = 1024;
        wchar_t TempBuffer[DATA_MAX_LEN];
        ::GetModuleFileName(NULL, TempBuffer, DATA_MAX_LEN);
        wstring wStr = TempBuffer;     
        string strFullFileName;
        Convert.UnicodeToUTF8(wStr, strFullFileName);
        string::size_type  pos = 0;
        pos = strFullFileName.find_last_of("\\");
        if (pos == string::npos)
            strPath = strFullFileName;
        else
            strPath = strFullFileName.substr(0, pos);
        strPath += "\\";
#endif
        return strPath.c_str();
    }

    string getExtName(const string & strFilename, bool bDot)
    {
        string str;
        string::size_type pos = strFilename.rfind(".");
        if (string::npos != pos) {
            string::size_type next = strFilename.size();
            if (!bDot)
                pos += 1;
            str = strFilename.substr(pos, next - pos);
            
        } 
        else {
            str = strFilename;
        }
        transform(str.begin(), str.end(), str.begin(), (int(*)(int))tolower);
        return str;
    }

    bool isExistFile(string & filename)
    {
        bool bResult = true;
        fstream _file;
        _file.open(filename.c_str(), ios::in | ios::binary);
        if (!_file)
        {
            bResult = false;
        }
        return bResult;
    }

    void saveFile(string & strFilename, string & strData)
    {
        ofstream ouF;
        ouF.open(strFilename.c_str(), std::ios::binary);
        ouF.write(strData.data(), strData.size());
        ouF.flush();
        ouF.close();
    }

    bool readFile(string & filename, string & strData)
    {
        bool bResul = false;
        std::size_t size;
        ifstream file(filename, ios::in | ios::binary | ios::ate);
        if (!file.is_open()) {
            return  bResul;
        }
        size =(size_t) file.tellg();
        if (size > 0) {
            file.seekg(0, ios::beg);
            strData.resize(size);
            char *buffer = reinterpret_cast<char *>(&strData[0]);
            file.read(buffer, size);

            bResul = true;
        }
        file.close();
        return bResul;
    }

    void trimstr(string & s)
    {
        s.erase(s.find_last_not_of(" \n\r\t") + 1);
        s.erase(0, s.find_first_not_of(" \n\r\t"));
        return;
    }

    string trim(string & str)
    {
        string & s = str;
        s.erase(s.find_last_not_of(" \n\r\t") + 1);
        s.erase(0, s.find_first_not_of(" \n\r\t"));
        return s;
    }    

    unsigned char ToHex(unsigned char x)
    {
        return  x > 9 ? x + 55 : x + 48;
    }

    unsigned char FromHex(unsigned char x)
    {
        unsigned char y;
        if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
        else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
        else if (x >= '0' && x <= '9') y = x - '0';
        else assert(0);
        return y;
    }

    std::string UrlEncode(const std::string& str)
    {
        std::string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            if (isalnum((unsigned char)str[i]) ||
                (str[i] == '-') ||
                (str[i] == '_') ||
                (str[i] == '.') ||
                (str[i] == '~'))
                strTemp += str[i];
            else if (str[i] == ' ')
                strTemp += "+";
            else
            {
                strTemp += '%';
                strTemp += ToHex((unsigned char)str[i] >> 4);
                strTemp += ToHex((unsigned char)str[i] % 16);
            }
        }
        return strTemp;
    }

    std::string UrlDecode(const std::string& str)
    {
        std::string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            if (str[i] == '+')
                strTemp += ' ';
            else if (str[i] == '%')
            {
                assert(i + 2 < length);
                unsigned char high = FromHex((unsigned char)str[++i]);
                unsigned char low = FromHex((unsigned char)str[++i]);
                strTemp += high * 16 + low;
            }
            else strTemp += str[i];
        }
        return strTemp;
    }

    void getGmtTime(string & strTime)
    {
        time_t rawTime;
        struct tm* timeInfo;
        char szTemp[40] = { 0 };

        time(&rawTime);
        timeInfo = localtime(&rawTime);
        strftime(szTemp, sizeof(szTemp), "%a, %d %b %Y %H:%M:%S", timeInfo);
        strTime = szTemp;
    }

    bool getKV(string & k, string & v, const string & strInfo)
    {
        vector<string> vecTemp;
        split(vecTemp, strInfo, ":");
        if (vecTemp.size() < 2) {
            return false;
        }
        k = vecTemp[0];
        v = vecTemp[1];
        trimstr(k);
        trimstr(v);
        return true;
    }

    void split(vector<string> & vecInfo, const string & src, string pattern, size_t maxsplit)
    {
        vecInfo.clear();
        size_t Start = 0;
        size_t end = 0;
        string sub;
        while (Start < src.size()) {
            end = src.find_first_of(pattern, Start);
            if (string::npos == end || vecInfo.size() >= maxsplit) {
                sub = src.substr(Start);
                vecInfo.push_back(sub);
                return;
            }
            sub = src.substr(Start, end - Start);
            vecInfo.push_back(sub);
            Start = end + 1;
        }
        return;
    }

    string getAbsolutePath(string & strPath, string & curPath)
    {
        string strRet = strPath;
        string strFlag = "/";       
#ifdef  WIN_OS
        strFlag = "\\";        
#endif
        vector<string> vecPath;
        split(vecPath, strPath, strFlag);
        if (vecPath.size() > 0) {
            if (("." == vecPath[0]) || (".." == vecPath[0])) {
                strRet = curPath + strPath;
            }         
        }
        return strRet;
    }

    string replace(const string & str, const string & src, const string & dest)
    {
        string ret;

        string::size_type pos_begin = 0;
        string::size_type pos = str.find(src);
        while (pos != string::npos)
        {            
            ret.append(str.data() + pos_begin, pos - pos_begin);
            ret += dest;
            pos_begin = pos + 1;
            pos = str.find(src, pos_begin);
        }
        if (pos_begin < str.length())
        {
            ret.append(str.begin() + pos_begin, str.end());
        }
        return ret;
    }

    void eraseLastPath(string & strPath)
    {
        if (strPath.empty())
            return;
        
        string desFlag = "/";
        char cValue = '/';
#ifdef  WIN_OS
        desFlag = "\\";
        cValue = '\\';       
#endif 
        if (*strPath.rbegin() == cValue) {
            string::size_type pos = strPath.size() - 1;
            strPath.erase(pos, 1);           
        }
    }

    void updatePath(string & strPath, bool bLast)
    {
        if (strPath.empty())
            return;
        string srcFlag = "\\";
        string desFlag = "/";
        char cValue = '/';

#ifdef  WIN_OS
        srcFlag = "/";
        desFlag = "\\";
        cValue = '\\';
#endif 
        strPath = replace(strPath, srcFlag, desFlag);
        if (bLast) {
            if (*strPath.rbegin() != cValue) {
                strPath += desFlag;
            }           
        }        
    }

    void eraseStr(string & strData, string flag)
    {
        string::size_type pos = 0;
        do {
            pos = strData.find(flag, pos);
            if (string::npos == pos) {
                break;
            }
            strData.erase(pos, flag.size());
        } while (true);
        return;
    }

    string combinPath(string curPath, string urlPath)
    {
        string strPath;
        eraseLastPath(curPath);
        updatePath(urlPath, false);
        strPath = curPath + urlPath;
        return strPath;
    }

    int getSize(vector<string> & vecInfo)
    {
        int len = 0;
        for (auto & str : vecInfo) {
            len += static_cast<int>(str.size());
        }
        return len;
    }


} //namespace UTIL_SELF

