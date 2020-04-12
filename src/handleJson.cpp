#include "common.h"
#include "handleJson.h"
#include "log.h"
#include "util.h"

using namespace rapidjson;

CHandleJson::CHandleJson()
{
}


CHandleJson::~CHandleJson()
{

}

bool CHandleJson::extractJsonConfig(JSON_CONFIG_INFO & info, string & strJson)
{
    
    auto pDoc(UTIL_SELF::make_unique<rapidjson::Document>());
    rapidjson::Document & doc = *pDoc;
    doc.Parse<0>(strJson.c_str());
    if (doc.HasParseError())
    {
        LOG_ERROR(" it's error to analysis");
        return false;
    }
    if (json_check_is_array(doc, JSON_remote_mode)) {
        rapidjson::Value & jsonResult = doc[JSON_remote_mode];         
        for (rapidjson::SizeType i = 0; i < jsonResult.Size(); i++) {
            REMOTE_MODE_INFO remoteInfo;
            rapidjson::Value & refValue = jsonResult[i];
            remoteInfo.ip_v = json_check_uint32(refValue, JSON_ip_v);
            remoteInfo.remote_ip = json_check_string(refValue, JSON_remote_ip);
            remoteInfo.port = json_check_uint32(refValue, JSON_remote_port);
            remoteInfo.live_time = json_check_uint32(refValue, JSON_live_time);
            info.remote_mode.push_back(remoteInfo);
        }       
    }
    if (json_check_is_array(doc, JSON_server)) {
        rapidjson::Value & jsonResult = doc[JSON_server];
        for (rapidjson::SizeType i = 0; i < jsonResult.Size(); i++) {
            rapidjson::Value & refValue = jsonResult[i];
            SERVICE_CONFIG_INFO srvConfig;
            srvConfig.server_name = json_check_string(refValue, JSON_server_name);
            srvConfig.www_root = json_check_string(refValue, JSON_www_root);
            UTIL_SELF::updatePath(srvConfig.www_root);
            srvConfig.index = json_check_string(refValue, JSON_index); 
            srvConfig.https = json_check_bool(refValue, JSON_https);
            if (json_check_value_ptr(refValue, JSON_https_config)) {
                rapidjson::Value & refHttps = refValue[JSON_https_config];
                HTTPS_INFO & httpInfo = srvConfig.https_config;
                httpInfo.path = json_check_string(refHttps, JSON_certificate_path);
                UTIL_SELF::updatePath(httpInfo.path);
                httpInfo.fileKey = json_check_string(refHttps, JSON_private_file);
                httpInfo.fileCa= json_check_string(refHttps, JSON_ca_file);    
                httpInfo.fileRoot = json_check_string(refHttps, JSON_root_file);
            }
            srvConfig.fastcgi = json_check_bool(refValue, JSON_fastcgi);
            if (json_check_value_ptr(refValue, JSON_fastcgi_config)) {
                rapidjson::Value & refFcgi = refValue[JSON_fastcgi_config];
                FCGI_CONFIG_INFO & fcgiInfo = srvConfig.fastcgi_config;
                fcgiInfo.fcgi_ip = json_check_string(refFcgi, JSON_fcgi_ip);
                fcgiInfo.fcgi_port = json_check_uint32(refFcgi, JSON_fcgi_port);
                fcgiInfo.file_path = json_check_string(refFcgi, JSON_file_path);
                UTIL_SELF::updatePath(fcgiInfo.file_path, false);
                if (json_check_is_array(refFcgi, JSON_ext_name)) {
                    rapidjson::Value & refName = refFcgi[JSON_ext_name];
                    for (rapidjson::SizeType i = 0; i < refName.Size(); i++) {
                        string ext = refName[i].GetString();
                        transform(ext.begin(), ext.end(), ext.begin(), (int(*)(int))tolower);
                        fcgiInfo.ext_name.push_back(ext);
                    }
                }

            }
            srvConfig.wsgi = json_check_bool(refValue, JSON_wsgi);
            if (json_check_value_ptr(refValue, JSON_wsgi_config)) {
                rapidjson::Value & refWsgi = refValue[JSON_wsgi_config];
                WSGI_INFO & wsgiInfo = srvConfig.wsgi_config;
                wsgiInfo.wgsi_path = json_check_string(refWsgi, JSON_wgsi_path);
                UTIL_SELF::updatePath(wsgiInfo.wgsi_path, false);
                wsgiInfo.wgsi_app = json_check_string(refWsgi, JSON_wgsi_app);
            }
            info.server.push_back(srvConfig);
        }
    }

    return true;
}