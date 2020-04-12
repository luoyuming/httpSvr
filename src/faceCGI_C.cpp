#include "faceCGI_C.h"
#include "log.h"
#include "util.h"

CFCGI_C::CFCGI_C()
{
	m_requestId = 0;
	cgi = (FastCgi_t *)malloc(sizeof(FastCgi_t));
	if (cgi) {
		FastCgi_init(cgi);
	}
	
}
CFCGI_C::~CFCGI_C() 
{
	if (cgi) {
		FastCgi_finit(cgi);
	}
	free(cgi);
}

bool CFCGI_C::getEnvValue(string & value, std::unordered_map<string, string> & mField, string flag)
{
	bool bResult = false;
	auto it = mField.find(flag);
	if (mField.end() != it) {
		value = it->second;
		bResult = true;
	}
	return bResult;
}

bool CFCGI_C::fcgiRequest(PACKAGE_INFO & info)
{
    auto & config = info.serviceConfig->fastcgi_config;
	bool bResult = true;
	vector<CGI_PARAM_INFO> vecParam;
	CGI_PARAM_INFO param;
	param.value = "GET";
	param.name = "REQUEST_METHOD";
	if (HTTP_POST == info.commandID) {
		param.value = "POST";
	}
	vecParam.push_back(param);

	if (getEnvValue(param.value, info.mField, "cookie")) {
		param.name = "HTTP_COOKIE";
		vecParam.push_back(param);
	}

	param.name = "CONTENT_TYPE";
	param.value = "text/html";
	getEnvValue(param.value, info.mField, "content-type");
	vecParam.push_back(param);

	param.name = "CONTENT_LENGTH";
	param.value = "0";
	getEnvValue(param.value, info.mField, "content-length");
	vecParam.push_back(param);
	int length = std::atoi(param.value.c_str());

	param.name = "REQUEST_METHOD";
	param.value = "GET";
	if (HTTP_POST == info.commandID) {
		param.value = "POST";
	}
	vecParam.push_back(param);
	vector<string> vecSplit;
	UTIL_SELF::split(vecSplit, info.uri, "?");
	int i = 0;
	for (auto & item : vecSplit) {		
		if (0 == i) {
			param.name = "SCRIPT_FILENAME";
			param.value = config.file_path;
			param.value += item;
            LOG_ERROR("%s", param.value.c_str());
		}
		else if (1 == i) {
			param.name = "QUERY_STRING";
			param.value = item;
		}
		vecParam.push_back(param);
		i++;
	}
	setRequestId(cgi, getRequestId());
    if (!startConnect(cgi, config.fcgi_ip.c_str(), config.fcgi_port)) {
        info.strResp = "fault to connect fastcgi server ip=" + config.fcgi_ip;
        info.strResp += ("  port=" + std::to_string(config.fcgi_port));
        return false;
    }
	if (0 > sendStartRequestRecord(cgi)) {
		LOG_ERROR("sendStartRequestRecord");
		return false;
	}
	for (auto & item : vecParam) {		
		cgi->requestId_ = getRequestId();
		if (sendParams(cgi, item.name.data(), item.value.data()) < 0) {
			LOG_ERROR("sendParams");
			return false;
		}
	}
	cgi->requestId_ = getRequestId();
	if (sendEndRequestRecord(cgi) < 0) {
		LOG_ERROR("sendEndRequestRecord");
		return false;
	}
	if (HTTP_POST == info.commandID) {
		
		int total = FCGI_HEADER_LEN * 2 + length;
		string strStdIn;
		strStdIn.resize(total);
		FCGI_Header head;
		makeHeader(&head, FCGI_STDIN, getRequestId(), length, 0);
		int i = 0;
		memcpy(&strStdIn[i], &head, FCGI_HEADER_LEN);
		i += FCGI_HEADER_LEN;
		memcpy(&strStdIn[i], info.strBody.data(), length);
		i += length;
		makeHeader(&head, FCGI_STDIN, getRequestId(), 0, 0);
		memcpy(&strStdIn[i], &head, FCGI_HEADER_LEN);

		if (cgi_write(cgi->sockfd_, &strStdIn[0], total) < 0)	{
			LOG_ERROR("FCGI_STDIN");
			return false;
		}
	}

	bResult = readFromPhp(cgi, info.strResp);
	if (!bResult) {
		info.strResp = "404  fastcgi is wrong!!";
	}
	return bResult;
}



int CFCGI_C::getRequestId()
{
	m_requestId++;
	if (m_requestId >= 65535) {
		m_requestId = 1;
	}
	return m_requestId;
}

