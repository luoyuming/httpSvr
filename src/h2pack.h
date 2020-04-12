#ifndef _HPACK_H_
#define _HPACK_H_

#include "common.h"
#include "h2huffman.h"
#include "h2Common.h"

/*
Indexed Header Field
     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
   | 1 |        Index (7+)         |
   +---+---------------------------+

Literal Header Field with Incremental Indexing -- Indexed Name
     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
   | 0 | 1 |      Index (6+)       |
   +---+---+-----------------------+
   | H |     Value Length (7+)     |
   +---+---------------------------+
   | Value String (Length octets)  |
   +-------------------------------+
   
Literal Header Field with Incremental Indexing -- New Name
     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
   | 0 | 1 |           0           |
   +---+---+-----------------------+
   | H |     Name Length (7+)      |
   +---+---------------------------+
   |  Name String (Length octets)  |
   +---+---------------------------+
   | H |     Value Length (7+)     |
   +---+---------------------------+
   | Value String (Length octets)  |
   +-------------------------------+

Literal Header Field without Indexing -- Indexed Name
     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
   | 0 | 0 | 0 | 0 |  Index (4+)   |
   +---+---+-----------------------+
   | H |     Value Length (7+)     |
   +---+---------------------------+
   | Value String (Length octets)  |
   +-------------------------------+

Literal Header Field without Indexing -- New Name
     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
   | 0 | 0 | 0 | 0 |       0       |
   +---+---+-----------------------+
   | H |     Name Length (7+)      |
   +---+---------------------------+
   |  Name String (Length octets)  |
   +---+---------------------------+
   | H |     Value Length (7+)     |
   +---+---------------------------+
   | Value String (Length octets)  |
   +-------------------------------+

Literal Header Field Never Indexed -- Indexed Name
     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
   | 0 | 0 | 0 | 1 |  Index (4+)   |
   +---+---+-----------------------+
   | H |     Value Length (7+)     |
   +---+---------------------------+
   | Value String (Length octets)  |
   +-------------------------------+

Literal Header Field Never Indexed -- New Name
     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
   | 0 | 0 | 0 | 1 |       0       |
   +---+---+-----------------------+
   | H |     Name Length (7+)      |
   +---+---------------------------+
   |  Name String (Length octets)  |
   +---+---------------------------+
   | H |     Value Length (7+)     |
   +---+---------------------------+
   | Value String (Length octets)  |
   +-------------------------------+
*/





struct HTTP2_HEADER_FIELD
{
	union 
	{
		struct
		{
            std::uint8_t index : 7;
            std::uint8_t code : 1;
		} indexed;
		struct
		{
            std::uint8_t index : 6;
            std::uint8_t code : 2;
		} with_indexing;
		struct
		{
            std::uint8_t index : 4;
            std::uint8_t code : 4;
		} without_indexing;
        struct
		{
            std::uint8_t index : 4;
            std::uint8_t code : 4;
		} never_indexed;
        std::uint8_t type;
	} tag;	
    
};

enum class index_type_e
{
    type_indexed = 0,
    type_with_indexing_indexed_name,
    type_with_indexing_new_name,
    type_without_indexing_indexed_name,
    type_without_indexing_new_name,
    type_never_indexed_indexed_name,
    type_never_indexed_new_name,
    type_error
};

struct HTTP2_HEADER_INDEX
{
    index_type_e index_type;
    std::uint32_t index;
    string name;
    string value;
};

class hpack
{
public:
    hpack();
    int parse(HTTP2_HEADER_FIELD* field, int len);
    int getNextIndex(std::shared_ptr<HTTP2_HEADER_INDEX> & headerIndex, int pos);
    void buildStatus();
    void buildField(string name, string value, map<int, pair<string, string> > & table);
    int getEncodeSize();
    void copyField(char *pData);
private:
    int encode_http2_header_string(HTTP2_HEADER_STRING* header_string_buf, int length, char** string_ptr);
    int decode_http2_header_string(HTTP2_HEADER_STRING* header_string_buf, int* length, char** string_ptr);


    int decode_http2_header_index(char* header_index_buf, int prefix, int* length);
    int encode_http2_header_index(char* header_index_buf, int prefix, int length);
    int64_t decode_integer(uint32_t &dst, const uint8_t *buf_start, const uint8_t *buf_end, uint8_t n);
    int64_t encode_integer(uint8_t *buf_start, const uint8_t *buf_end, uint32_t value, uint8_t n);

    bool getInteger(int & parsed, HTTP2_HEADER_INDEX & header, HTTP2_HEADER_FIELD* curret_field, int index);
private:
    std::vector<std::shared_ptr<HTTP2_HEADER_INDEX>> m_decodedHeaders;
    std::list<string>                                m_encodeInfo;
};
#endif /* _HPACK_H_ */