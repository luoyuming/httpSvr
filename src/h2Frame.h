#pragma once
#include "common.h"
#include "h2Common.h"
#include "util.h"
#include "log.h"


static const string HTTP2_PERFACE = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
#define HTTP2_PREFACE_LEN       24

#define HTTP2_HTTP_1_1_ALPN "\x8http/1.1"
#define HTTP2_HTTP_1_1_ALPN_LEN (sizeof(HTTP2_HTTP_1_1_ALPN) - 1)
#define HTTP2_PROTO_ALPN "\x2h2"
#define HTTP2_PROTO_ALPN_LEN (sizeof(HTTP2_PROTO_ALPN) - 1)
#define HTTP2_H2   "h2"


enum class HTTP2_CONNECT_STATE {
    HTTP2_ALPN_NONE = 1,
    HTTP2_ALPN_PREFACE,
    HTTP2_ALPN_CONNECT,   
};


#define HTTP2_DEFAULT_WINDOW_SIZE   65535
#define HTTP2_MAX_FRAME_LEN     16384

enum class stream_state_e
{
    stream_idle = 0,
    stream_open,
    stream_reserved_local,
    stream_reserved_remote,
    stream_half_closed_remote,
    stream_half_closed_local,
    stream_closed
};
