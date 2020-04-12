#pragma once
#include "msgDef.h"
#include "common.h"
#include "json/document.h"  
#include "json/prettywriter.h"
#include "json/stringbuffer.h"  
#include "json/rapidjson.h" 
#include "json/reader.h" 
#include "json/writer.h" 




#define  JSON_remote_mode					"remote_mode"
#define  JSON_ip_v					         "ip_v"
#define  JSON_remote_ip					    "remote_ip"
#define  JSON_remote_port					"remote_port"
#define  JSON_live_time					    "live_time"
#define  JSON_server					    "server"
#define  JSON_server_name					"server_name"
#define  JSON_www_root					    "www_root"
#define  JSON_index					        "index"
#define  JSON_https				            "https"
#define  JSON_https_config				    "https_config"
#define  JSON_certificate_path				"certificate_path"
#define  JSON_private_file					"private_file"
#define  JSON_ca_file					    "ca_file"
#define  JSON_root_file					    "root_file"
#define  JSON_fastcgi				        "fastcgi"
#define  JSON_fastcgi_config				"fastcgi_config"
#define  JSON_ext_name					    "ext_name"
#define  JSON_fcgi_ip					    "fcgi_ip"
#define  JSON_fcgi_port					    "fcgi_port"
#define  JSON_file_path					    "file_path"
#define  JSON_wsgi					        "wsgi"
#define  JSON_wsgi_config					"wsgi_config"
#define  JSON_wgsi_path					    "wgsi_path"
#define  JSON_wgsi_app					    "wgsi_app"




// 基础变量的校验  

#define json_check_is_bool(value, strKey) (value.HasMember(strKey) && value[strKey].IsBool())  
#define json_check_is_string(value, strKey) (value.HasMember(strKey) && value[strKey].IsString())  
#define json_check_is_int32(value, strKey) (value.HasMember(strKey) && value[strKey].IsInt())  
#define json_check_is_uint32(value, strKey) (value.HasMember(strKey) && value[strKey].IsUint())  
#define json_check_is_int64(value, strKey) (value.HasMember(strKey) && value[strKey].IsInt64())  
#define json_check_is_uint64(value, strKey) (value.HasMember(strKey) && value[strKey].IsUint64())  
#define json_check_is_float(value, strKey) (value.HasMember(strKey) && value[strKey].IsFloat())  
#define json_check_is_double(value, strKey) (value.HasMember(strKey) && value[strKey].IsDouble())  

#define json_check_is_number(value, strKey) (value.HasMember(strKey) && value[strKey].IsNumber())  
#define json_check_is_array(value, strKey) (value.HasMember(strKey) && value[strKey].IsArray())  


// 得到对应类型的数据，如果数据不存在则得到一个默认值  
#define json_check_bool(value, strKey) (json_check_is_bool(value, strKey) && value[strKey].GetBool())  
#define json_check_string(value, strKey) (json_check_is_string(value, strKey) ? value[strKey].GetString() : "")  
#define json_check_int32(value, strKey) (json_check_is_int32(value, strKey) ? value[strKey].GetInt() : 0)  
#define json_check_uint32(value, strKey) (json_check_is_uint32(value, strKey) ? value[strKey].GetUint() : 0)  
#define json_check_int64(value, strKey) (json_check_is_int64(value, strKey) ? ((value)[strKey]).GetInt64() : 0)  
#define json_check_uint64(value, strKey) (json_check_is_uint64(value, strKey) ? ((value)[strKey]).GetUint64() : 0)  
#define json_check_float(value, strKey) (json_check_is_float(value, strKey) ? ((value)[strKey]).GetFloat() : 0)  
#define json_check_double(value, strKey) (json_check_is_double(value, strKey) ? ((value)[strKey]).GetDouble() : 0)  

// 得到Value指针  
#define json_check_value_ptr(value, strKey) (((value).HasMember(strKey)) ? &((value)[strKey]) : nullptr)  

class CHandleJson
{
public:
    CHandleJson();
    ~CHandleJson();
    
public:  
    bool extractJsonConfig(JSON_CONFIG_INFO & info, string & strJson);
	
};