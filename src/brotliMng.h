#pragma once
#include "common.h"
#include "brotli/decode.h"
#include "brotli/encode.h"


class  CBrotliMng {
public:
	CBrotliMng();
	~CBrotliMng();
	bool brotliDecode(string & strDst, string & strSrc);
	bool brotliDecodeEx(string & strDst, string & strSrc);
	bool brotliEncode(string & strDst, string & strSrc);
private:
	
	
};