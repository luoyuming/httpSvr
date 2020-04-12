/*
 * codeDeflate.h
 *
 * 
 */

#ifndef COMPRESSION_H_
#define COMPRESSION_H_
#include "common.h"

#include <zlib.h>
#include <zconf.h>
#include <iostream>
#include <vector>



class Compression
{
#define COMPRESSION_LEVEL Z_DEFAULT_COMPRESSION
#define BUFFER_SIZE 16384
	typedef std::vector<unsigned char> StlVecUnChar;
	enum CompressLevel
	{
		NO_COMPRESSION = Z_NO_COMPRESSION,
		BEST_SPEED = Z_BEST_SPEED,
		BEST_COMPRESSION = Z_BEST_COMPRESSION,
		DEFAULT_COMPRESSION = Z_DEFAULT_COMPRESSION
	};
public:
	bool code(string & dest, string & src);
	bool decode(string & dest, string & src);
	Compression();
	Compression(bool isCom, CompressLevel level = DEFAULT_COMPRESSION);
	virtual ~Compression();
private:
	int Init();

    int Deflate(const StlVecUnChar &inStr, StlVecUnChar &outStr);
    int Inflate(const StlVecUnChar &inStr, StlVecUnChar &outStr);

private:

    z_stream      m_zstream;               // Stream structure used by zlib
    bool          m_IsCompress;            // True: compress. False: decompress
    unsigned char m_bufferIn[BUFFER_SIZE]; // Input buffer for zlib
    unsigned char m_bufferOut[BUFFER_SIZE];// Output Buffer for zlib
    const size_t     m_bufferInCapa;          // Input buffer capacity
    const size_t     m_bufferOutCapa;         // Output buffer capacity
    CompressLevel	m_compressLevel;         // Compress level
};

#endif /* COMPRESSION_H_ */



