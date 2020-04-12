#include "h2pack.h"
#include "util.h"
#include "log.h"
#include "h2huffman.h"


hpack::hpack()
{

}

int hpack::encode_http2_header_string(HTTP2_HEADER_STRING* header_string_buf, int length, char** string_ptr)
{
    int64_t integer_len = encode_integer((uint8_t *)header_string_buf, (uint8_t *)header_string_buf + MAX_BYTES_OF_LENGTH, length, 7);
    *string_ptr = integer_len < 0 ? NULL : (char*)header_string_buf + integer_len;

    return (int)integer_len;
}

int hpack::decode_http2_header_string(HTTP2_HEADER_STRING* header_string_buf, int* length, char** string_ptr)
{

    uint32_t dst = 0;
    int64_t integer_len = decode_integer(dst, (uint8_t *)header_string_buf, (uint8_t *)header_string_buf + MAX_BYTES_OF_LENGTH, 7);
    *length = dst;
    *string_ptr = integer_len < 0 ? NULL : (char*)header_string_buf + integer_len;

    return (int)integer_len;
}

int64_t hpack::encode_integer(uint8_t *buf_start, const uint8_t *buf_end, uint32_t value, uint8_t n)
{
    if (buf_start >= buf_end)
        return -1;

    uint8_t *p = buf_start;

    if (value < (static_cast<uint32_t>(1 << n) - 1)) {
        *(p++) = value;
    }
    else {
        *(p++) = (1 << n) - 1;
        value -= (1 << n) - 1;
        while (value >= 128) {
            if (p >= buf_end) {
                return -1;
            }
            *(p++) = (value & 0x7F) | 0x80;
            value = value >> 7;
        }
        if (p + 1 >= buf_end) {
            return -1;
        }
        *(p++) = value;
    }
    return p - buf_start;
}

int64_t hpack::decode_integer(uint32_t &dst, const uint8_t *buf_start, const uint8_t *buf_end, uint8_t n)
{
    if (buf_start >= buf_end) {
        return -1;
    }

    const uint8_t *p = buf_start;

    dst = (*p & ((1 << n) - 1));
    if (dst == static_cast<uint32_t>(1 << n) - 1) {
        int m = 0;
        do {
            if (++p >= buf_end)
                return -1;

            uint32_t added_value = *p & 0x7f;
            if ((UINT32_MAX >> m) < added_value) {
                // Excessively large integer encodings - in value or octet
                // length - MUST be treated as a decoding error.
                return -1;
            }
            dst += added_value << m;
            m += 7;
        } while (*p & 0x80);
    }

    return p - buf_start + 1;
}

int hpack::encode_http2_header_index(char* header_index_buf, int prefix, int length)
{
    int64_t integer_len = encode_integer((uint8_t *)header_index_buf, (uint8_t *)header_index_buf + MAX_BYTES_OF_LENGTH, length, prefix);
    return (int)integer_len;
}

int hpack::decode_http2_header_index(char* header_index_buf, int prefix, int* length)
{

    uint32_t dst = 0;
    int64_t integer_len = decode_integer(dst, (uint8_t *)header_index_buf, (uint8_t *)header_index_buf + MAX_BYTES_OF_LENGTH, prefix);
    *length = dst;

    return (int)integer_len;
}

bool hpack::getInteger(int & parsed, HTTP2_HEADER_INDEX & header, HTTP2_HEADER_FIELD* curret_field, int index)
{  
    int f_index = 0;
    char** header_string_ptr = nullptr;
    int integer_len = decode_http2_header_index((char*)&(curret_field->tag.type), index, &f_index);
    if (integer_len < 0) {
        LOG_ERROR("error to decode header index");
        return false;
    }
    parsed += integer_len;
    header.index = f_index;
    //LOG_INFO("f_index %d  integer_len %d ", header.index, integer_len);
    return true;
}

int hpack::parse(HTTP2_HEADER_FIELD* field, int len)
{
    int ret = 0;
    int parsed = 0;
    HTTP2_HEADER_FIELD *pTemp = field;
    while (parsed < len) {
        auto headerIndex = std::make_shared<HTTP2_HEADER_INDEX>();
        HTTP2_HEADER_INDEX & header = *headerIndex;
        string & refValue = header.value;
        string & refName = header.name;

        header.index_type = index_type_e::type_error;
        HTTP2_HEADER_FIELD* curret_field = (HTTP2_HEADER_FIELD*)((char*)(pTemp + parsed));

        // 1xxx xxxx
        if (curret_field->tag.indexed.code == 1) {         
            header.index_type = index_type_e::type_indexed;
            if (!getInteger(parsed, header, curret_field, 7)) {
                return -1;
            }
            m_decodedHeaders.push_back(headerIndex);

            //LOG_INFO("1xxx xxxx indexed %d", header.index);
        }
        //01xx xxxx
        else {
            if (curret_field->tag.with_indexing.code == 1) {

                header.index_type = curret_field->tag.with_indexing.index > 0 ? index_type_e::type_with_indexing_indexed_name : index_type_e::type_with_indexing_new_name;
                if (!getInteger(parsed, header, curret_field, 6)) {
                    return -1;
                }               
                //LOG_INFO("01xx xxxx indexed %d", header.index);  
            }
            else {
                if (!getInteger(parsed, header, curret_field, 4)) {
                    return -1;
                }
                //LOG_INFO("#### xxxx indexed %d", header.index);
                if (curret_field->tag.without_indexing.code == 0)
                {
                    header.index_type = curret_field->tag.without_indexing.index > 0 ? index_type_e::type_without_indexing_indexed_name : index_type_e::type_without_indexing_new_name;
                    header.index = curret_field->tag.without_indexing.index;
                }
                else if (curret_field->tag.never_indexed.code == 1)
                {
                    header.index_type = curret_field->tag.never_indexed.index > 0 ? index_type_e::type_never_indexed_indexed_name : index_type_e::type_never_indexed_new_name;
                                       
                }
                else
                {
                    LOG_INFO("!! wrong index type");
                    return -1;
                }
            }
            HTTP2_HEADER_STRING* header_string = (HTTP2_HEADER_STRING*)((char*)(pTemp + parsed));
            int string_len = 0;
            char* string_ptr = NULL;
            int integer_len = decode_http2_header_string(header_string, &string_len, &string_ptr);
            if (integer_len < 0)
            {
                LOG_ERROR("error to decode header index");
                return -1;
            }
            parsed += integer_len + string_len;
            if (header.index > 0) {
                if (header_string->h == 1) //Huffman
                {
                    QYT_NS::NODE* h_node;
                    QYT_NS::hf_init(&h_node);
                    refValue.resize(MAX_HUFFMAN_BUFF_LEN(string_len));
                    char* out_buff = (char*)&refValue[0];                    
                    memset(out_buff, 0, MAX_HUFFMAN_BUFF_LEN(string_len));
                    int out_size = QYT_NS::hf_string_decode(h_node, (unsigned char*)string_ptr, string_len, out_buff, MAX_HUFFMAN_BUFF_LEN(string_len));
                    if (out_size < 0)
                    { 
                        QYT_NS::hf_finish(h_node);
                        return -1;
                    }                    
                    QYT_NS::hf_finish(h_node);                  
                }
                else
                {                   
                    refValue.resize(string_len);
                    char* out_buff = (char*)&refValue[0];
                    memcpy(out_buff, string_ptr, string_len);                   
                }
            }
            else if (header.index == 0) {
                if (header_string->h == 1) //Huffman
                {
                    QYT_NS::NODE* h_node;
                    QYT_NS::hf_init(&h_node);
                    refName.resize(MAX_HUFFMAN_BUFF_LEN(string_len));
                    char* out_buff = (char*)&refName[0];
                    memset(out_buff, 0, MAX_HUFFMAN_BUFF_LEN(string_len));
                    int out_size = QYT_NS::hf_string_decode(h_node, (unsigned char*)string_ptr, string_len, out_buff, MAX_HUFFMAN_BUFF_LEN(string_len));
                    if (out_size < 0)
                    {                      
                        QYT_NS::hf_finish(h_node);
                        return -1;
                    }
                    QYT_NS::hf_finish(h_node);                   
                }
                else
                {
                    refName.resize(string_len);
                    char* out_buff = (char*)&refName[0];                  
                    memcpy(out_buff, string_ptr, string_len); 
                   
                }

                HTTP2_HEADER_STRING* header_string2 = (HTTP2_HEADER_STRING*)((char*)(pTemp + parsed));

                int string_len2 = 0;
                char* string_ptr2 = NULL;
                int integer_len2 = decode_http2_header_string(header_string2, &string_len2, &string_ptr2);
                if (integer_len2 < 0)
                    return -1;
                parsed += integer_len2 + string_len2;
                if (header_string2->h == 1) //Huffman
                {
                    QYT_NS::NODE* h_node;
                    QYT_NS::hf_init(&h_node);
                    refValue.resize(MAX_HUFFMAN_BUFF_LEN(string_len2));
                    char* out_buff = (char*)&refValue[0];
                    memset(out_buff, 0, MAX_HUFFMAN_BUFF_LEN(string_len2));
                    int out_size = QYT_NS::hf_string_decode(h_node, (unsigned char*)string_ptr2, string_len2, out_buff, MAX_HUFFMAN_BUFF_LEN(string_len2));
                    if (out_size < 0)
                    {                       
                        QYT_NS::hf_finish(h_node);
                        return -1;
                    }
                    QYT_NS::hf_finish(h_node);                  
                }
                else
                {
                    refValue.resize(string_len2);
                    char* out_buff = (char*)&refValue[0];
                    memcpy(out_buff, string_ptr2, string_len2);                   
                }
            }

            m_decodedHeaders.push_back(headerIndex);
        }
    }

    //for (auto & item : m_decodedHeaders) {
        //LOG_ERROR("index %d  name:%s value:%s", item.index, item.name.c_str(), item.value.c_str());
    //}

    return ret;
}

int hpack::getNextIndex(std::shared_ptr<HTTP2_HEADER_INDEX> & headerIndex, int pos)
{
    int size = static_cast<int>(m_decodedHeaders.size());
    if (pos >= size) {
        return -1;
    }
    headerIndex = m_decodedHeaders[pos];
    return (pos + 1);
}

void hpack::buildStatus()
{   
    HTTP2_HEADER_FIELD field;    
    int len = encode_http2_header_index((char*)&(field.tag.type), 7, 8);
    field.tag.indexed.code = 1;
    if (len <= 0) {
        LOG_ERROR("len less than 0 %d ", len);
    }
    else {
        string strTemp;
        strTemp.resize(len);
        memcpy(&strTemp[0], &field, len);
        m_encodeInfo.push_back(strTemp);
    }
    return;
}

void hpack::copyField(char *pData)
{
    int i = 0;
    for (auto & str : m_encodeInfo) {
        memcpy(&pData[i], str.data(), str.size());
        i += static_cast<int>(str.size());
    }
    return;
}

int hpack::getEncodeSize()
{
    int size = 0;
    for (auto & str : m_encodeInfo) {
        size += static_cast<int>(str.size());
    }
    return size;
}

void hpack::buildField(string name, string value, map<int, pair<string, string> > & table)
{
    int len = 0;
    HTTP2_HEADER_STRING* hdr_string = nullptr;
    std::uint32_t hdr_string_len = 0;

    HTTP2_HEADER_FIELD field;
    index_type_e index_type = index_type_e::type_never_indexed_new_name;
    len = encode_http2_header_index((char*)&(field.tag.type), 4, 0);
    if (len < 0) {
        LOG_ERROR("len less than 0 %d ", len);
        return;
    }
    field.tag.never_indexed.code = 1;

    for (auto & item : table) {
        if (name == item.second.first) {
            index_type = index_type_e::type_with_indexing_indexed_name;
            len = encode_http2_header_index((char*)&(field.tag.type), 6, item.first);
            if (len < 0) {
                LOG_ERROR("name(%s) len less than 0 %d ", name.c_str(), len);
                return;
            }
            field.tag.with_indexing.code = 1;
            if (value == item.second.second) {
                index_type = index_type_e::type_indexed;
                len = encode_http2_header_index((char*)&(field.tag.type), 7, item.first);
                if (len < 0) {
                    LOG_ERROR("name(%s) len less than 0 %d ", value.c_str(), len);
                    return;
                }
                field.tag.indexed.code = 1;
            }
            break;
        }
    }

    if (index_type_e::type_with_indexing_indexed_name == index_type) {
        const char*  string_buf = value.c_str();
        int string_len = value.length();

        hdr_string = (HTTP2_HEADER_STRING*)malloc(sizeof(HTTP2_HEADER_STRING) + MAX_BYTES_OF_LENGTH + MAX_HUFFMAN_BUFF_LEN(string_len));

        int out_len = 0;
        unsigned char* out_buff = (unsigned char*)malloc(MAX_HUFFMAN_BUFF_LEN(string_len));
        memset(out_buff, 0, MAX_HUFFMAN_BUFF_LEN(string_len));

        QYT_NS::NODE* h_node;
        QYT_NS::hf_init(&h_node);
        QYT_NS::hf_string_encode(string_buf, string_len, 0, out_buff, &out_len);
        QYT_NS::hf_finish(h_node);

        char* string_ptr = NULL;
        int integer_len = encode_http2_header_string(hdr_string, out_len, &string_ptr);
        hdr_string->h = 1;
        memcpy(string_ptr, out_buff, out_len);
        free(out_buff);
        hdr_string_len = integer_len + out_len;
    }
    else if (index_type == index_type_e::type_never_indexed_new_name) {

        const char*  string_buf = name.c_str();
        int string_len = name.length();

        const char*  string_buf2 = value.c_str();
        int string_len2 = value.length();

        hdr_string = (HTTP2_HEADER_STRING*)malloc(sizeof(HTTP2_HEADER_STRING) + MAX_BYTES_OF_LENGTH + MAX_HUFFMAN_BUFF_LEN(string_len) + MAX_HUFFMAN_BUFF_LEN(string_len2));

        int out_len = 0;
        unsigned char* out_buff = (unsigned char*)malloc(MAX_HUFFMAN_BUFF_LEN(string_len));
        memset(out_buff, 0, MAX_HUFFMAN_BUFF_LEN(string_len));

        QYT_NS::NODE* h_node;
        QYT_NS::hf_init(&h_node);
        QYT_NS::hf_string_encode(string_buf, string_len, 0, out_buff, &out_len);
        QYT_NS::hf_finish(h_node);

        char* string_ptr = NULL;
        int integer_len = encode_http2_header_string(hdr_string, out_len, &string_ptr);

        hdr_string->h = 1;

        memcpy(string_ptr, out_buff, out_len);

        free(out_buff);

        hdr_string_len = integer_len + out_len;


        HTTP2_HEADER_STRING* hdr_string2 = (HTTP2_HEADER_STRING*)((char*)hdr_string + integer_len + out_len);

        int out_len2 = 0;
        unsigned char* out_buff2 = (unsigned char*)malloc(MAX_HUFFMAN_BUFF_LEN(string_len2));
        memset(out_buff2, 0, MAX_HUFFMAN_BUFF_LEN(string_len2));

        QYT_NS::NODE* h_node2;
        QYT_NS::hf_init(&h_node2);
        QYT_NS::hf_string_encode(string_buf2, string_len2, 0, out_buff2, &out_len2);
        QYT_NS::hf_finish(h_node2);

        char* string_ptr2 = NULL;
        int integer_len2 = encode_http2_header_string(hdr_string2, out_len2, &string_ptr2);

        hdr_string2->h = 1;

        memcpy(string_ptr2, out_buff2, out_len2);

        free(out_buff2);

        hdr_string_len += integer_len2 + out_len2;
    }
    int total = len + hdr_string_len;
    string strTemp;
    strTemp.resize(total);
    int i = 0;
    memcpy(&strTemp[i], &field, len);
    i += len;
    if (hdr_string && hdr_string_len > 0) {
        memcpy(&strTemp[i], hdr_string, hdr_string_len);
    }

    m_encodeInfo.push_back(strTemp);

    if (hdr_string)
        free(hdr_string);
    return;
}