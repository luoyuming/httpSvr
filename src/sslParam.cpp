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


#include "sslParam.h"
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


CSslParam::CSslParam(SSL * s)
    :rbio_(nullptr), wbio_(nullptr)
{     
        rbio_ = BIO_new(BIO_s_mem());
        assert(rbio_ != nullptr);
        wbio_ = BIO_new(BIO_s_mem());
        assert(wbio_ != nullptr);
        SSL_set_bio(s, rbio_, wbio_);
}

CSslParam::~CSslParam()
{
    BIO_free(rbio_);
    BIO_free(wbio_);
}

BIO * CSslParam::rbio() { return rbio_; }
BIO * CSslParam::wbio() { return wbio_; }

