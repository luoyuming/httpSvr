#pragma once
#include "singleton.h"
#include "common.h"
#include "msgDef.h"
#include "socketService.h"
#include "fcgi.h"

struct CGI_PARAM_INFO {
	string name;
	string value;
};

class CFCGI_C {

public:
	CFCGI_C();
	~CFCGI_C();
	bool fcgiRequest(PACKAGE_INFO & info);
private:	
	int getRequestId();
	bool getEnvValue(string & value, std::unordered_map<string, string> & mField, string flag);


private:
	int m_requestId;
	FastCgi_t * cgi;
};