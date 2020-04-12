#include "log.h"
#include "funDef.h"

CGLog * CGLog::m_pInstance = nullptr;
CGLog::CGLog(void)
{

}

CGLog::~CGLog(void)
{
	google::ShutdownGoogleLogging();
}

int32_t CGLog::InitGLog(const char * argv0)
{	
#ifdef LOG_OUT_PRINT

	string dir_log = "log";
	string log_fatal = "./log/log_fatal_";
	string log_error = "./log/log_error_";
	string log_warning = "./log/log_warning_";
	string log_info = "./log/log_info_";
#ifdef WIN_OS 
	dir_log = "log";
	log_fatal = "log\\log_fatal_";
	log_error = "log\\log_error_";
	log_warning = "log\\log_warning_";
	log_info = "log\\log_info_";
#endif

	mkdir(dir_log.c_str(), 0755);
	google::InitGoogleLogging(argv0);
	google::SetStderrLogging(google::GLOG_INFO); //设置级别高于 google::INFO 的日志同时输出到屏幕
	google::SetLogDestination(google::GLOG_FATAL, log_fatal.c_str()); // 设置 google::FATAL 级别的日志存储路径和文件名前缀
	google::SetLogDestination(google::GLOG_ERROR, log_error.c_str()); //设置 google::ERROR 级别的日志存储路径和文件名前缀
	google::SetLogDestination(google::GLOG_WARNING, log_warning.c_str()); //设置 google::WARNING 级别的日志存储路径和文件名前缀
	google::SetLogDestination(google::GLOG_INFO, log_info.c_str()); //设置 google::INFO 级别的日志存储路径和文件名
	
	FLAGS_max_log_size = 100; //最大日志大小为 100MB
	FLAGS_stop_logging_if_full_disk = true; //当磁盘被写满时，停止日志输出
	FLAGS_alsologtostderr = true;
	FLAGS_colorlogtostderr = true; //设置输出到屏幕的日志显示相应颜色
	
#endif	
	return 0;
}

void CGLog::GLogMsg(const char * funName, int lineNum, uint32_t nLogSeverity, const char *format, ...)
{
#ifdef LOG_OUT_PRINT
	string strTemp;
	strTemp.resize(MAX_BUFF_LOG_LEN+1);
    strTemp[MAX_BUFF_LOG_LEN] = '\0';
	char *pLogBuff = const_cast<char*>(strTemp.data());
	va_list arg_ptr;
	va_start(arg_ptr, format);
	vsnprintf(pLogBuff,MAX_BUFF_LOG_LEN, format, arg_ptr);
	switch (nLogSeverity)
	{
	case 0:
		LOG(INFO) << "[" << funName << ":" << std::to_string(lineNum) <<"] " << pLogBuff;       
		break;
	case 1:
		LOG(WARNING) << "[" << funName << ":" << std::to_string(lineNum) << "] " << pLogBuff;
		break;
	case 2:
		LOG(ERROR) << "[" << funName << ":" << std::to_string(lineNum) << "] " << pLogBuff;
		break;
	case 3:
		LOG(FATAL) << "[" << funName << ":" << std::to_string(lineNum) << "] " << pLogBuff;
		break;
	default:
		break;
	}
	va_end(arg_ptr);	
	
#endif
	return;
}

