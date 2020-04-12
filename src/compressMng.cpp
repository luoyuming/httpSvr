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

#include "compressMng.h"

CompressMng::CompressMng()
{
    m_br = std::make_shared<CBrotliMng>();
    m_gzip = std::make_shared<CGzip>();
    m_deflate = std::make_shared<Compression>();
}

CompressMng::~CompressMng()
{

}

void CompressMng::encode(std::shared_ptr<PACKAGE_INFO> & pPkgInfo)
{
    encode(*pPkgInfo);
}

void CompressMng::encode(PACKAGE_INFO & info)
{
    string strData;
    string key = "accept-encoding";
    auto it = info.mField.find(key);
    if (info.mField.end() != it) {
        string str = it->second;
        transform(str.begin(), str.end(), str.begin(), (int(*)(int))tolower);
        string::size_type pos = str.find("br");
        if (string::npos != pos) {
            if (m_br->brotliEncode(strData, info.strResp)) {
                info.strResp = strData;
                info.contentEncoding = "Content-Encoding: br";

                return;
            }
            else {
                LOG_ERROR("brotliEncode is fault");
            }
        }

        pos = str.find("gzip");
        if (string::npos != pos) {
            if (m_gzip->codeGzip(strData, info.strResp)) {
                info.strResp = strData;
                info.contentEncoding = "Content-Encoding: gzip";

                return;
            }
            else {
                LOG_ERROR("CGzip is fault");
            }
        }

        pos = str.find("deflate");
        if (string::npos != pos) {
            if (m_deflate->code(strData, info.strResp)) {
                info.strResp = strData;
                info.contentEncoding = "Content-Encoding: deflate";

                return;
            }
            else {
                LOG_ERROR("deflate is fault");
            }
        }
    }
    
    if (m_gzip->codeGzip(strData, info.strResp)) {
        info.strResp = strData;
        info.contentEncoding = "Content-Encoding: gzip";
    }
    return;
}