/*
*  12797202@qq.com   name: luoyuming mobile 13925236752
*
* Copyright (c) 2020 www.dswd.net
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
* LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
* OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
* WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#include "dispatch.h"
#include "log.h"
#include "handleXml.h"
#include "configMng.h"
#include "faceCGI_C.h"
#include "compressMng.h"

using namespace std::placeholders;

CDispatch::CDispatch()
{
    m_contentType = std::make_shared<ContentTypeMng>();
	initFun();

}

CDispatch::~CDispatch()
{

}

void CDispatch::initFun()
{
	std::uint32_t i = 0;
	Command httpMothed[] = {
		{ HTTP_POST, std::bind(&CDispatch::postMothed, this, _1) },
		{ HTTP_GET, std::bind(&CDispatch::getMothed, this, _1) },
	};
	for (i = 0; i < sizeof(httpMothed) / sizeof(httpMothed[0]); ++i)
	{
		m_FunHttpMothed[httpMothed[i].commandID] = httpMothed[i];
	}

	//////////////////////////////////////////////////////
	UrlMothed httpPost[] = {
		{ "/", std::bind(&CDispatch::httpIndex, this, _1) },
	};

	for (i = 0; i < sizeof(httpPost) / sizeof(httpPost[0]); ++i)
	{
		m_FunHttpPost[httpPost[i].commandID] = httpPost[i];
	}

	////////////////////////////////////////////////////////////////
	UrlMothed httpGet[] = {
		{"/", std::bind(&CDispatch::httpIndex, this, _1)},	
        {"/version", std::bind(&CDispatch::httpVersion, this, _1)},
	
	};

	for (i = 0; i < sizeof(httpGet) / sizeof(httpGet[0]); ++i)
	{
		m_FunHttpGet[httpGet[i].commandID] = httpGet[i];
	}
}

void CDispatch::dispatchInfo(PACKAGE_INFO & info)
{	
	auto it = m_FunHttpMothed.find(info.commandID);
	if (it != m_FunHttpMothed.end())
	{
		auto & cmd = it->second;
		cmd.callbackFun(info);
	}	
	return;
}

void CDispatch::getMothed(PACKAGE_INFO & info)
{	
	disptMothed(m_FunHttpGet, info);
	return;
}

void CDispatch::postMothed(PACKAGE_INFO & info)
{	
	disptMothed(m_FunHttpPost, info);
	return;
}

bool CDispatch::multipartFormData(PACKAGE_INFO & info, string & strFieldInfo)
{
    string strBoundaryFlag = "boundary=";
    string::size_type pos = strFieldInfo.find(strBoundaryFlag);
    if (string::npos == pos) {
        return false;
    }

    pos += strBoundaryFlag.size();
    string::size_type next = strFieldInfo.size();
    string strBoundary = strFieldInfo.substr(pos, next-pos);
    string padFlag = "--";
    string BoundaryBegin = padFlag + strBoundary;
    string BoundaryEnd = BoundaryBegin + padFlag;
    pos = info.strBody.rfind(BoundaryEnd);
    if (string::npos == pos)
    {
        return false;
    }   
    handleFormData(info, BoundaryBegin);
    return true;
}

void CDispatch::handleFormData(PACKAGE_INFO & info, string & strBoundary)
{
    string::size_type pos = 0, next = 0;
    do {
        auto filedInfo = std::make_shared<FORM_DATA_INFO>();
        next = getFormData(*filedInfo, info.strBody, pos, strBoundary);
        if (string::npos == next)
        {            
            break;
        }       
        info.vecFormData.push_back(filedInfo);
        pos = next;
    } while (true);
   
    return;
}

string::size_type CDispatch::getFormData(FORM_DATA_INFO & formData, string & strInfo, string::size_type pos, string & strBoundary)
{
    string::size_type next = string::npos;
    pos = strInfo.find(strBoundary, pos);
    if (string::npos == pos)
    {
        return string::npos;
    }
    pos += strBoundary.size();
    next = strInfo.find(strBoundary, pos);
    if (string::npos == next)
    {
        next = strInfo.size();
        if (next <= pos)
        {
            return string::npos;
        }
    }
    string dataFlag = strCRLNCRLN;
    string::size_type mid = strInfo.find(dataFlag, pos);
    if (string::npos == mid)
    {
        return string::npos;
    }
    if (mid >= next)
    {
        return string::npos;
    }
    string strField = strInfo.substr(pos, mid - pos);
    if (!getFormDataName(formData.name, strField))
    {
        return string::npos;
    }
    getFormFileName(formData.fileName, strField);

    mid += dataFlag.size();
    string::size_type len = next - mid - strCRLN.size();
    formData.data = strInfo.substr(mid, len);
    return next;
}


bool CDispatch::getFormDataName(string & strName, string & strInfo)
{   
    string strFlag = "name=\"";
    string::size_type pos = strInfo.find(strFlag);
    if (string::npos == pos)
    {
        return false;
    }
    pos += strFlag.size();
    strFlag = "\"";
    string::size_type next = strInfo.find(strFlag, pos);
    if (string::npos == next)
    {
        return false;
    }
    strName = strInfo.substr(pos, next - pos);
    return true;
}

bool CDispatch::getFormFileName(string & strName, string & strInfo)
{   
    strName.clear();
    string strFlag = "filename=\"";
    string::size_type pos = strInfo.find(strFlag);
    if (string::npos == pos)
    {
        return false;
    }
    pos += strFlag.size();
    strFlag = "\"";
    string::size_type next = strInfo.find(strFlag, pos);
    if (string::npos == next)
    {
        return false;
    }
    strName = strInfo.substr(pos, next - pos);
    return true;
}

bool CDispatch::formUrlencodedData(PACKAGE_INFO & info)
{
    auto & mpInfo = info.mBodyUrlencoded;
    string key, value;
    vector<string> vecTemp;
    UTIL_SELF::split(vecTemp, info.strBody, "&");
    for (auto & item : vecTemp) {
        vector<string> vecKeyValue;
        UTIL_SELF::split(vecKeyValue, item, "=");
        if (vecKeyValue.size() >= 2) {
            key = UTIL_SELF::UrlDecode(vecKeyValue[0]);           
            value = UTIL_SELF::UrlDecode(vecKeyValue[1]);
            UTIL_SELF::trimstr(value);
            mpInfo[key] = value;
        }
    }
    return true;
}

bool CDispatch::httpFormData(PACKAGE_INFO & info)
{
    string ctFlag = "content-type";
    auto it = info.mField.find(ctFlag);
    if (info.mField.end() == it)
        return false;
    string strFiled = it->second;
    transform(strFiled.begin(), strFiled.end(), strFiled.begin(), (int(*)(int))tolower);    
    string flag = "multipart/form-data";
    string::size_type pos = strFiled.find(flag);
    if (string::npos != pos) {
        return multipartFormData(info, strFiled);
    }

    flag = "/x-www-form-urlencoded";
    pos = strFiled.find(flag);
    if (string::npos != pos) {
        return formUrlencodedData(info);
    }

    return true;
}

string CDispatch::getHostName(std::unordered_map<string, string> & Field)
{
    string hostField;
    auto it = Field.find("host");
    if (Field.end() != it) {
        hostField = it->second;
        string::size_type pos = it->second.find(":");
        if (string::npos != pos) {
            hostField = it->second.substr(0, pos);
        }
    }
    return hostField;
}

bool CDispatch::handleFCGI(PACKAGE_INFO & info)
{   
    bool bFind = false;
    string extName = UTIL_SELF::getExtName(info.url, true);
    vector<string> & vecExtName = info.serviceConfig->fastcgi_config.ext_name;
    for (auto & ext : vecExtName) {
        if (extName == ext) {
            bFind = true;
            break;
        }
    }
    if (!bFind) {
        return false;
    }

    info.strResp = "404 not found";
    info.contentType = "Content-Type: text/plain";
    auto fcgi(std::make_shared<CFCGI_C>());
    if (fcgi->fcgiRequest(info))
    {
        info.bFCGIResp = true;
    }
    return true;
}

void CDispatch::disptMothed(std::map<string, UrlMothed> & dispFun, PACKAGE_INFO & info)
{    
    if (HTTP_POST == info.commandID) {
        httpFormData(info);
    }

    info.serviceConfig = CMngConfigS->getJsonPtr()->getServiceConfig(getHostName(info.mField));
    if (info.serviceConfig->fastcgi) {
        if (handleFCGI(info)) {
            return;
        }
    }

    bool bFind = false;
    auto it = dispFun.find(info.url);
    if (it != dispFun.end())
    {
        bFind = true;
    }
    if (bFind) {
        auto & cmd = it->second;
        cmd.callbackFun(info);
    }
    else if (HTTP_GET == info.commandID) {
        info.strResp.clear();
        string strPath = info.curPath;

        strPath = UTIL_SELF::getAbsolutePath(info.serviceConfig->www_root, strPath);
        info.fileNameAsyn = UTIL_SELF::combinPath(strPath, info.url);

        if (!UTIL_SELF::isExistFile(info.fileNameAsyn)) {
            info.fileNameAsyn.clear();
            LOG_WARNING("not to find the file %s(%s)", info.fileNameAsyn.c_str(), info.url.c_str());
        }
      
        if (!info.fileNameAsyn.empty()) {
            string extName = UTIL_SELF::getExtName(info.fileNameAsyn);
            if (!extName.empty())
                info.contentType = m_contentType->getContentType(extName);
        }
       
    }
    if (info.strResp.empty() && info.fileNameAsyn.empty()) {
        httpGet50x(info);
    }
    if (info.contentType.empty()) {
        string extName = UTIL_SELF::getExtName(info.fileNameAsyn);
        if (!extName.empty()) {
            info.contentType = m_contentType->getContentType(extName);
        }
    }
    
    return;
}



void CDispatch::httpGet50x(PACKAGE_INFO & info)
{
    info.contentType = "Content-Type: text/html;charset=utf-8";
    info.fileNameAsyn = info.curPath + "50x.html";
    if (!UTIL_SELF::isExistFile(info.fileNameAsyn)) {
        LOG_ERROR("not exist the file (%s)", info.fileNameAsyn.c_str());
        info.fileNameAsyn.clear();
    }
    return;
}

void CDispatch::httpVersion(PACKAGE_INFO & info)
{
    info.contentType = "Content-Type: text/plain";
    info.strResp = HTTP_SVR_VERSION;
}

void CDispatch::httpIndex(PACKAGE_INFO & info)
{    
    info.contentType = "Content-Type: text/html;charset=utf-8";
    string strPath = info.curPath;
    strPath = UTIL_SELF::getAbsolutePath(info.serviceConfig->www_root, strPath);

    info.fileNameAsyn = strPath + "index.html";
    if (!UTIL_SELF::isExistFile(info.fileNameAsyn)) {
        LOG_ERROR("not exist the file (%s)", info.fileNameAsyn.c_str());
        info.fileNameAsyn.clear();        
    }
    
	return;
}



