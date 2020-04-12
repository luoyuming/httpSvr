#pragma once
#include "common.h"


#ifdef LOG_OUT_PRINT
#include <glog/logging.h>
#include <glog/log_severity.h> 
#endif

static const int MAX_BUFF_LOG_LEN = 1024;
class CGLog
{
	CGLog(void);
	~CGLog(void);	
public:
	int32_t InitGLog(const char * argv0);	
	void GLogMsg(const char * funName, int lineNum, uint32_t nLogSeverity, const char *format, ...);
public:
	static CGLog * Instance()
	{
		if (!m_pInstance)
		{
			m_pInstance = new CGLog();
		}
		return m_pInstance;
	}
 
private:
	static CGLog * m_pInstance;

};
 
 
#define LOG_INIT(...) CGLog::Instance()->InitGLog(__VA_ARGS__);
#define LOG_INFO(...) CGLog::Instance()->GLogMsg(__FILE__,__LINE__,0,__VA_ARGS__);
#define LOG_WARNING(...) CGLog::Instance()->GLogMsg(__FILE__,__LINE__,1,__VA_ARGS__);
#define LOG_ERROR(...) CGLog::Instance()->GLogMsg(__FILE__,__LINE__,2, __VA_ARGS__);
#define LOG_FATAL(...) CGLog::Instance()->GLogMsg(__FILE__,__LINE__,3, __VA_ARGS__);

