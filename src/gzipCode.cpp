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

#include "gzipCode.h"
#include "log.h"
#include "util.h"
#include "codeDeflate.h"
#include "brotliMng.h"


CGzip::CGzip()
{

}

CGzip::~CGzip()
{

}


bool CGzip::codeGzip(string & dest, string & src)
{
	if (src.empty())
		return false;
	
	uLong size = static_cast<uLong>(src.size());
	Bytef *pSrc = reinterpret_cast<Bytef *>(&src[0]);
	string strTemp;
	uLong outLen = size*2;
	strTemp.resize(outLen);	
	Bytef *pDest = reinterpret_cast<Bytef *>(&strTemp[0]);
	if (gzcompress(pSrc, size, pDest, &outLen) < 0) {
		LOG_ERROR("fault to gzcomress)");
		return false;
	}
	dest.clear();
	dest.append(strTemp.data(), (string::size_type)outLen);
	//LOG_INFO("len(%d)-->len(%d)", size, outLen);
	
	return true;
}

bool CGzip::decodeGzip(string & dest, string & src)
{
	if (src.empty())
		return false;

	uLong size = static_cast<uLong>(src.size());
	Bytef *pSrc = reinterpret_cast<Bytef *>(&src[0]);
	string strTemp;
	uLong outLen = size * 8;
	strTemp.resize(outLen);
	Bytef *pDest = reinterpret_cast<Bytef *>(&strTemp[0]);
	if (gzdecompress(pSrc, size, pDest, &outLen) < 0) {
		LOG_ERROR("fault to gzdecompress)");
		return false;
	}
	dest.clear();
	dest.append(strTemp.data(), (string::size_type)outLen);
	//cout << "decodeGzip len:" << dest.size() << endl;
	return true;
}

int CGzip::gzcompress(Bytef *data, uLong ndata, Bytef  *zdata, uLong *nzdata)
{
	z_stream c_stream;
	int err = 0;
	if (data && ndata > 0) {
		c_stream.zalloc = NULL;
		c_stream.zfree = NULL;
		c_stream.opaque = NULL;

		//只有设置为MAX_WBITS + 16才能在在压缩文本中带header和trailer

		if (deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
			MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) 
			return-1;

		c_stream.next_in = data;
		c_stream.avail_in = ndata;
		c_stream.next_out = zdata;
		c_stream.avail_out = *nzdata;
		while (c_stream.avail_in != 0 && c_stream.total_out < *nzdata) {
			if (deflate(&c_stream, Z_NO_FLUSH) != Z_OK) {
				deflateEnd(&c_stream);
				return-1;
			}
		}

		if (c_stream.avail_in != 0)
			return c_stream.avail_in;

		for (;;) {
			if ((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END)
				break;
			if (err != Z_OK) {
				deflateEnd(&c_stream);
				return-1;
			}

		}

		if (deflateEnd(&c_stream) != Z_OK) 
			return-1;

		*nzdata = c_stream.total_out;
		return 0;

	}

	return-1;
}
int CGzip::gzdecompress(Bytef *zdata, uLong nzdata, Bytef *data, uLong *ndata)
{
	int err = 0;
	z_stream d_stream = { 0 }; /* decompression stream */

	static char dummy_head[2] = {
		0x8 + 0x7 * 0x10,
		(((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
	};

	d_stream.zalloc = NULL;
	d_stream.zfree = NULL;
	d_stream.opaque = NULL;
	d_stream.next_in = zdata;
	d_stream.avail_in = 0;
	d_stream.next_out = data;

	//只有设置为MAX_WBITS + 16才能在解压带header和trailer的文本

	if (inflateInit2(&d_stream, MAX_WBITS + 16) != Z_OK)
		return-1;

	//if(inflateInit2(&d_stream, 47) != Z_OK) return -1;

	while (d_stream.total_out < *ndata && d_stream.total_in < nzdata) {
		d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */

		if ((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END)
			break;

		if (err != Z_OK) {
			if (err == Z_DATA_ERROR) {
				d_stream.next_in = (Bytef*)dummy_head;
				d_stream.avail_in = sizeof(dummy_head);
				if ((err = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK) {
					inflateEnd(&d_stream);
					return -1;

				}

			}
			else {
				inflateEnd(&d_stream);
				return -1;
			}
		}

	}

	if (inflateEnd(&d_stream) != Z_OK) 
		return-1;

	*ndata = d_stream.total_out;
	return 0;
}

