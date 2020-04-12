#pragma once
#include "common.h"


namespace UTIL_SELF {
	template<typename T, typename... Args>
	std::unique_ptr<T> make_unique(Args&&... args) {
		return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}

#define VALUE_BREAK_IF(cond) if(cond) break
	inline bool setValue(string & value, vector<string> & vecInfo, int & i, int max)
	{
		if ((i >= max) && (!vecInfo.empty()))
		{
			return true;
		}
		value = vecInfo[i++];
		return false;
	}

#define SSTR( x ) static_cast< std::ostringstream & >(( std::ostringstream() << std::dec << x ) ).str()


	class Timer {
    public:
	    Timer() : m_begin(high_resolution_clock::now()) {}
	    void reset() { m_begin = high_resolution_clock::now(); }	
	    int64_t elapsed() const
	    {
		    return duration_cast<chrono::milliseconds>(high_resolution_clock::now() - m_begin).count();
	    }	
	    int64_t elapsed_micro() const
	    {
		    return duration_cast<chrono::microseconds>(high_resolution_clock::now() - m_begin).count();
	    }	
	    int64_t elapsed_nano() const
	    {
		    return duration_cast<chrono::nanoseconds>(high_resolution_clock::now() - m_begin).count();
	    }	
	    int64_t elapsed_seconds() const
	    {
		    return duration_cast<chrono::seconds>(high_resolution_clock::now() - m_begin).count();
	    }	
	    int64_t elapsed_minutes() const
	    {
		    return duration_cast<chrono::minutes>(high_resolution_clock::now() - m_begin).count();
	    }	
	    int64_t elapsed_hours() const
	    {
		    return duration_cast<chrono::hours>(high_resolution_clock::now() - m_begin).count();
	    }
    private:
	    time_point<high_resolution_clock> m_begin;
    };

    std::string strToHex(const std::string str, std::string separator = "");
    string getPwd();
    string getExtName(const string & strFilename, bool bDot=false);
    void saveFile(string & strFilename, string & strData);
    bool readFile(string & filename, string & strData);
    bool isExistFile(string & filename);
    void trimstr(string & s);
    string trim(string & str);
    unsigned char ToHex(unsigned char x);
    unsigned char FromHex(unsigned char x);
    std::string UrlEncode(const std::string& str);
    std::string UrlDecode(const std::string& str);
    void getGmtTime(string & strTime);
    void split(vector<string> & vecInfo, const string & src, string pattern, size_t maxsplit = string::npos);
    bool getKV(string & k, string & v, const string & strInfo);
    string getAbsolutePath(string & strPath, string & curPath);
    string replace(const string & str, const string & src, const string & dest);
    void updatePath(string & strPath, bool bLast=true);
    void eraseLastPath(string & strPath);
    void eraseStr(string & strData, string flag);
    string combinPath(string curPath, string urlPath);
    int getSize(vector<string> & vecInfo);

}


