#pragma once
#include "common.h"

/*
HTTP frame
All frames begin with a fixed 9-octet header followed by a variable-
length payload.

+-----------------------------------------------+
|                 Length (24)                   |
+---------------+---------------+---------------+
|   Type (8)    |   Flags (8)   |
+-+-------------+---------------+-------------------------------+
|R|                 Stream Identifier (31)                      |
+=+=============================================================+
|                   Frame Payload (0...)                      ...
+---------------------------------------------------------------+
*/

#define HTTP2_FRAME_TYPE_DATA             0x00
#define HTTP2_FRAME_TYPE_HEADERS          0x01
#define HTTP2_FRAME_TYPE_PRIORITY         0x02
#define HTTP2_FRAME_TYPE_RST_STREAM       0x03
#define HTTP2_FRAME_TYPE_SETTINGS         0x04
#define HTTP2_FRAME_TYPE_PUSH_PROMISE     0x05
#define HTTP2_FRAME_TYPE_PING             0x06
#define HTTP2_FRAME_TYPE_GOAWAY           0x07
#define HTTP2_FRAME_TYPE_WINDOW_UPDATE    0x08
#define HTTP2_FRAME_TYPE_CONTINUATION     0x09

#define HTTP2_NO_ERROR                    0x00
#define HTTP2_PROTOCOL_ERROR              0x01
#define HTTP2_INTERNAL_ERROR              0x02
#define HTTP2_FLOW_CONTROL_ERROR          0x03
#define HTTP2_SETTINGS_TIMEOUT            0x04
#define HTTP2_STREAM_CLOSED               0x05
#define HTTP2_FRAME_SIZE_ERROR            0x06
#define HTTP2_REFUSED_STREAM              0x07
#define HTTP2_CANCEL                      0x08
#define HTTP2_COMPRESSION_ERROR           0x09
#define HTTP2_CONNECT_ERROR               0x0a
#define HTTP2_ENHANCE_YOUR_CALM           0x0b
#define HTTP2_INADEQUATE_SECURITY         0x0c
#define HTTP2_HTTP_1_1_REQUIRED           0x0d


#define HTTP2_FRAME_FLAG_UNSET            0x00
#define HTTP2_FRAME_FLAG_END_STREAM	      0x01
#define HTTP2_FRAME_FLAG_SETTING_ACK	  0x01
#define HTTP2_FRAME_FLAG_PING_ACK	      0x01
#define HTTP2_FRAME_FLAG_END_HEADERS      0x04
#define HTTP2_FRAME_FLAG_PADDED	          0x08
#define HTTP2_FRAME_FLAG_PRIORITY	      0x20

#define HTTP2_FRAME_R_UNSET               0x0
#define HTTP2_FRAME_R_SET                 0x1

#define HTTP2_STREAM_DEPENDENCY_E_UNSET   0x0
#define HTTP2_STREAM_DEPENDENCY_E_SET     0x1

#define HTTP2_FRAME_IDENTIFIER_WHOLE     0x00

#define HTTP2_SETTINGS_HEADER_TABLE_SIZE        0x01
#define HTTP2_SETTINGS_ENABLE_PUSH              0x02
#define HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS   0x03
#define HTTP2_SETTINGS_INITIAL_WINDOW_SIZE      0x04
#define HTTP2_SETTINGS_MAX_FRAME_SIZE           0x05
#define HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE     0x06

//#pragma pack(push)
//#pragma pack(1)
#pragma pack(push,1)

#define HTTP2_FRAME_HEAD_LEN        9
struct HTTP2_FRAME
{
    union {
        std::uint8_t  len3b[3];
        std::uint32_t len24 : 24;
    } length;
    std::uint8_t type;
    std::uint8_t flags;
    uint32_t r : 1;
    uint32_t identifier : 31;
    char payload[0];
};


/* Maxximn padding length */
#define MAX_PADDING_LEN     255
/*
DATA frames
+---------------+
|Pad Length? (8)|
+---------------+-----------------------------------------------+
|                            Data (*)                         ...
+---------------------------------------------------------------+
|                           Padding (*)                       ...
+---------------------------------------------------------------+
*/

struct HTTP2_FRAME_DATA1
{
    uint8_t pad_length;
    char data_padding[0];
} ;

struct HTTP2_FRAME_DATA2
{
    char data[0];
};


/*
Setting Format
+-------------------------------+
| Identifier (16)               |
+-------------------------------+-------------------------------+
| Value (32)                                                    |
+---------------------------------------------------------------+
*/

/*
#define HTTP2_SETTINGS_HEADER_TABLE_SIZE         0x01
#define HTTP2_SETTINGS_ENABLE_PUSH               0x02
#define HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS    0x03
#define HTTP2_SETTINGS_INITIAL_WINDOW_SIZE       0x04
#define HTTP2_SETTINGS_MAX_FRAME_SIZE            0x05
#define HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE      0x06
*/

struct HTTP2_SETTINGS
{
    std::uint16_t identifier;
    std::uint32_t value;
};


/*
WINDOW_UPDATE Payload Format
+-+-------------------------------------------------------------+
|R| Window Size Increment (31)                                  |
+-+-------------------------------------------------------------+
*/
typedef struct
{
    std::uint32_t r : 1;
    std::uint32_t win_size : 31;
} HTTP2_FRAME_WINDOW_UPDATE;


/*
HEADERS Frame Payload
+---------------+
|Pad Length? (8)|
+-+-------------+-----------------------------------------------+
|E|                 Stream Dependency? (31)                     |
+-+-------------+-----------------------------------------------+
|  Weight? (8)  |
+-+-------------+-----------------------------------------------+
|                   Header Block Fragment (*)                 ...
+---------------------------------------------------------------+
|                           Padding (*)                       ...
+---------------------------------------------------------------+
*/

struct HTTP2_FRAME_HEADER
{
    std::uint8_t pad_length;
    std::uint32_t e : 1;
    std::uint32_t dependency : 31;
    std::uint8_t weight;
    char block_fragment_padding[0];
};

struct HTTP2_FRAME_HEADER_PAD
{
    std::uint8_t pad_length;
    char bottom[0];
};

struct HTTP2_FRAME_HEADER_WEIGHT
{
    std::uint32_t e : 1;
    std::uint32_t dependency : 31;
    std::uint8_t weight;
    char block_fragment_padding[0];
};

struct HTTP2_FRAME_HEADER_FRAGMENT
{
    unsigned char block_fragment[0];  
};


/*
GOAWAY Payload Format
+-+-------------------------------------------------------------+
|R| Last-Stream-ID (31)                                         |
+-+-------------------------------------------------------------+
| Error Code (32)                                               |
+---------------------------------------------------------------+
| Additional Debug Data (*)                                     |
+---------------------------------------------------------------+
*/
struct HTTP2_FRAME_GOAWAY
{
    uint32_t r : 1;
    uint32_t last_stream_id : 31;
    uint32_t error_code;
    char debug_data[0];
};




#define MAX_BYTES_OF_LENGTH 256
struct HTTP2_HEADER_STRING
{
    std::uint8_t len_prefix : 7;
    std::uint8_t h : 1;
    std::uint8_t len_value[0];
};


/*
PING Payload Format
+---------------------------------------------------------------+
|                                                               |
| Opaque Data (64)                                              |
|                                                               |
+---------------------------------------------------------------+
*/

struct HTTP2_FRAME_PING
{
    uint8_t data[8];
} ;



#pragma pack(pop)



/*
+-------+-----------------------------+---------------+
| Index | Header Name                 | Header Value  |
+-------+-----------------------------+---------------+
| 1     | :authority                  |               |
| 2     | :method                     | GET           |
| 3     | :method                     | POST          |
| 4     | :path                       | /             |
| 5     | :path                       | /index.html   |
| 6     | :scheme                     | http          |
| 7     | :scheme                     | https         |
| 8     | :status                     | 200           |
| 9     | :status                     | 204           |
| 10    | :status                     | 206           |
| 11    | :status                     | 304           |
| 12    | :status                     | 400           |
| 13    | :status                     | 404           |
| 14    | :status                     | 500           |
| 15    | accept-charset              |               |
| 16    | accept-encoding             | gzip, deflate |
| 17    | accept-language             |               |
| 18    | accept-ranges               |               |
| 19    | accept                      |               |
| 20    | access-control-allow-origin |               |
| 21    | age                         |               |
| 22    | allow                       |               |
| 23    | authorization               |               |
| 24    | cache-control               |               |
| 25    | content-disposition         |               |
| 26    | content-encoding            |               |
| 27    | content-language            |               |
| 28    | content-length              |               |
| 29    | content-location            |               |
| 30    | content-range               |               |
| 31    | content-type                |               |
| 32    | cookie                      |               |
| 33    | date                        |               |
| 34    | etag                        |               |
| 35    | expect                      |               |
| 36    | expires                     |               |
| 37    | from                        |               |
| 38    | host                        |               |
| 39    | if-match                    |               |
| 40    | if-modified-since           |               |
| 41    | if-none-match               |               |
| 42    | if-range                    |               |
| 43    | if-unmodified-since         |               |
| 44    | last-modified               |               |
| 45    | link                        |               |
| 46    | location                    |               |
| 47    | max-forwards                |               |
| 48    | proxy-authenticate          |               |
| 49    | proxy-authorization         |               |
| 50    | range                       |               |
| 51    | referer                     |               |
| 52    | refresh                     |               |
| 53    | retry-after                 |               |
| 54    | server                      |               |
| 55    | set-cookie                  |               |
| 56    | strict-transport-security   |               |
| 57    | transfer-encoding           |               |
| 58    | user-agent                  |               |
| 59    | vary                        |               |
| 60    | via                         |               |
| 61    | www-authenticate            |               |
+-------+-----------------------------+---------------+
*/