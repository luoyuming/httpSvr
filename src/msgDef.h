#pragma once
#include "common.h"
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

static const string MAIN_MODE_RUN = "1";
#define DEFAULT_THREAD_NUM  4

static const string IP_V4_TYPE = "4";
static const string IP_V6_TYPE = "6";
struct XML_SYSTEM_CONFIG {    
    string  run_mode;
    string  ip_v;
    string tcp_port;
    string http_port;
    string https_port;
    string thread_events;
    string queue_capacity;
};


struct REMOTE_MODE_INFO {
    int  ip_v;
    string remote_ip;
    int port;
    int live_time;
};

struct HTTPS_INFO {
    string path;
    string fileKey;
    string fileCa;
    string fileRoot;
};

struct FCGI_CONFIG_INFO {
    vector<string>  ext_name;
    string fcgi_ip;
    int fcgi_port;
    string file_path;
};

struct WSGI_INFO {
    string wgsi_path;
    string wgsi_app;  
};

struct SERVICE_CONFIG_INFO {
    string server_name;
    string www_root;
    string index;
    bool https;
    HTTPS_INFO https_config;
    bool fastcgi;
    FCGI_CONFIG_INFO fastcgi_config;
    bool wsgi;
    WSGI_INFO wsgi_config;
};

struct JSON_CONFIG_INFO {
    vector<REMOTE_MODE_INFO>        remote_mode;
    vector<SERVICE_CONFIG_INFO>     server;
};


enum class  IP_TYPE {
    IP_V4 = 1,
    IP_V6,
};

enum class  PORT_TYPE {
    SOCKET_TCP =1,
    SOCKET_HTTP,
    SOCKET_HTTPS,
};

static const string TRACE_MOTHED = "TRACE";
static const string OPTIONS_MOTHED = "OPTIONS";
static const string CONNECT_MOTHED = "CONNECT";
static const string DELETE_MOTHED = "DELETE";
static const string PUT_MOTHED = "PUT";
static const string HEAD_MOTHED = "HEAD";
static const string GET_MOTHED = "GET";
static const string POST_MOTHED = "POST";
static const int HTTP_POST = 1;
static const int HTTP_GET = 2;
static const int HTTP_HEAD = 3;
static const int HTTP_PUT = 4;
static const int HTTP_DELETE = 5;
static const int HTTP_CONNECT = 6;
static const int HTTP_OPTIONS = 7;
static const int HTTP_TRACE = 8;

static const string strSPACE = " ";
static const string strCRLN = "\r\n";
static const string strCRLNCRLN = "\r\n\r\n";
#define CRLN_FLAG				strCRLN
#define HEAD_END_FLAG			strCRLNCRLN
#define HEAD_END_FLAG_ONE		"\n\n"
#define HEAD_END_FLAG_TWO		"\r\r"
#define ContentLength		"Content-Length: "






