
#include "codeDeflate.h"
#include <cstdio>
#include <cstring>
#include <cassert>


bool Compression::code(string & dest, string & src)
{
	bool bResult = false;
	StlVecUnChar vecDest;
	StlVecUnChar vecSrc(src.begin(), src.end());
	if (Z_OK == Init()) {
		if (Z_OK == Deflate(vecSrc, vecDest)) {
			dest.clear();
			dest.assign(vecDest.begin(), vecDest.end());
			bResult = true;
		}
	}
	return bResult;
}

bool Compression::decode(string & dest, string & src)
{
	bool bResult = false;
	StlVecUnChar vecDest;
	StlVecUnChar vecSrc(src.begin(), src.end());
	if (Z_OK == Init()) {
		if (Z_OK == Inflate(vecSrc, vecDest))
		{
			dest.clear();
			dest.assign(vecDest.begin(), vecDest.end());
			bResult = true;
		}
	}
	return bResult;
}


Compression::Compression()
	: m_zstream()
	, m_IsCompress(true)
	, m_bufferInCapa(BUFFER_SIZE)
	, m_bufferOutCapa(BUFFER_SIZE)
	, m_compressLevel(DEFAULT_COMPRESSION)
{

}

Compression::Compression(bool isCom, CompressLevel level)
	: m_zstream()
	, m_IsCompress(isCom)
	, m_bufferInCapa(BUFFER_SIZE)
	, m_bufferOutCapa(BUFFER_SIZE)
	, m_compressLevel(level)
{
}

Compression::~Compression() {
	// TODO Auto-generated destructor stub
}

int Compression::Init()
{
    int ret;
	m_zstream.zalloc = NULL;
	m_zstream.zfree  = NULL;
	m_zstream.opaque = NULL;
	if(m_IsCompress)
	{
		ret = deflateInit2_(&m_zstream, m_compressLevel, Z_DEFLATED, MAX_WBITS,
				MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY, ZLIB_VERSION, sizeof(z_stream));
	}
	else
	{
		ret = inflateInit2_(&m_zstream, MAX_WBITS, ZLIB_VERSION, sizeof(z_stream));
	}

	return ret;
}

int Compression::Deflate(const StlVecUnChar &inStr, StlVecUnChar &outStr)
{
    int ret;
    int flush;

    size_t lastByte = inStr.size ();
	int have  = 0;
    while (lastByte > 0)
	{
        if (lastByte > m_bufferInCapa)
		{
            memcpy (m_bufferIn, &inStr[inStr.size () - lastByte], m_bufferInCapa);
            lastByte           -= m_bufferInCapa;
            m_zstream.avail_in  = m_bufferInCapa;
            flush               = Z_NO_FLUSH;
		}
		else
		{
            memcpy (m_bufferIn, &inStr[inStr.size () - lastByte], lastByte);
            m_zstream.avail_in = lastByte;
            lastByte           = 0;
            flush              = Z_FINISH;
		}
        m_zstream.next_in  = m_bufferIn;

		do
		{
            m_zstream.avail_out = m_bufferOutCapa;
            m_zstream.next_out  = m_bufferOut;

			ret = deflate(&m_zstream, flush);
			assert(ret != Z_STREAM_ERROR);
            have = m_bufferOutCapa - m_zstream.avail_out;
            outStr.insert (outStr.end (), m_bufferOut, m_bufferOut + have);
		}while(m_zstream.avail_out == 0);
		assert(m_zstream.avail_in == 0);
	}

	// Finish deflate
	(void)deflateEnd(&m_zstream);
	return Z_OK;
}

int Compression::Inflate(const StlVecUnChar &inStr, StlVecUnChar &outStr)
{
    int ret = Z_OK;

	size_t lastByte = inStr.size();
	size_t have  = 0;
	while (lastByte > 0)
	{
        if (lastByte > m_bufferInCapa)
		{
            memcpy(m_bufferIn, &inStr[inStr.size () - lastByte], m_bufferInCapa);
            lastByte          -= m_bufferInCapa;
            m_zstream.avail_in = m_bufferInCapa;
		}
		else
		{
            memcpy(m_bufferIn, &inStr[inStr.size () - lastByte], lastByte);
			m_zstream.avail_in = lastByte;
			lastByte = 0;
		}
        m_zstream.next_in  = m_bufferIn;

		do
		{
            m_zstream.next_out  = m_bufferOut;
            m_zstream.avail_out = m_bufferOutCapa;

			ret = inflate(&m_zstream, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);
			switch(ret)
			{
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&m_zstream);
				return ret;
			}

            have  = m_bufferOutCapa - m_zstream.avail_out;
            outStr.insert (outStr.end (), m_bufferOut, m_bufferOut + have);
		}while(m_zstream.avail_out == 0);
	}

	(void)inflateEnd(&m_zstream);

	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}