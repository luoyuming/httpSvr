#pragma once
#include "common.h"
#include "msgDef.h"
#include "socketService.h"
#include "contentTypeMng.h"


class CDispatch {

	typedef std::function<void(PACKAGE_INFO & info)> CallbackFun;
	typedef struct _Command {
		int              commandID;
		CallbackFun      callbackFun;
	} Command;
	typedef struct _UrlMothed {
		string           commandID;
		CallbackFun      callbackFun;
	} UrlMothed;

	
public:
	CDispatch();
	~CDispatch();
	void dispatchInfo(PACKAGE_INFO & info);
private:
	void initFun();
	void postMothed(PACKAGE_INFO & info);
	void getMothed(PACKAGE_INFO & info);
	void disptMothed(std::map<string, UrlMothed> & dispFun, PACKAGE_INFO & info);

	bool httpFormData(PACKAGE_INFO & info);
    bool formUrlencodedData(PACKAGE_INFO & info);
    bool multipartFormData(PACKAGE_INFO & info, string & strFieldInfo);
    void handleFormData(PACKAGE_INFO & info, string & strBoundary);
    string::size_type getFormData(FORM_DATA_INFO & formData,string & strInfo, string::size_type pos, string & strBoundary);
    bool getFormDataName(string & strName, string & strInfo);
    bool getFormFileName(string & strName, string & strInfo);

    string getHostName(std::unordered_map<string, string> & Field);
    bool handleFCGI(PACKAGE_INFO & info);
protected:
    void httpGet50x(PACKAGE_INFO & info);
	void httpVersion(PACKAGE_INFO & info);
	void httpIndex(PACKAGE_INFO & info);
	
private:
	std::map<int, Command>				    m_FunHttpMothed;
	std::map<string, UrlMothed>			    m_FunHttpGet;
	std::map<string, UrlMothed>			    m_FunHttpPost;	
	
    
    std::shared_ptr<ContentTypeMng>         m_contentType;
};