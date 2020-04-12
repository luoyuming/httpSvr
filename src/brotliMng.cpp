#include "brotliMng.h"
#include "log.h"
#include "util.h"

CBrotliMng::CBrotliMng()
{
	
}

CBrotliMng::~CBrotliMng()
{
	
}

bool CBrotliMng::brotliDecodeEx(string & strDst, string & strSrc)
{	
	strDst.clear();
	const uint8_t* data = (const uint8_t*)&strSrc[0];
	size_t size = static_cast<int>(strSrc.size());
	size_t addend = 0;
	if (size > 0)
		addend = data[size - 1] & 7;
	const uint8_t* next_in = data;

	const int kBufferSize = 1024;
	uint8_t* buffer = (uint8_t*)malloc(kBufferSize);
	if (!buffer) {
		// OOM is out-of-scope here.
		return false;
	}
	/* The biggest "magic number" in brotli is 16MiB - 16, so no need to check
	the cases with much longer output. */
	const size_t total_out_limit = (addend == 0) ? (1 << 26) : (1 << 24);
	size_t total_out = 0;

	BrotliDecoderState* state = BrotliDecoderCreateInstance(0, 0, 0);
	if (!state) {
		free(buffer);
		return false;
	}

	if (addend == 0)
		addend = size;
	/* Test both fast (addend == size) and slow (addend <= 7) decoding paths. */
	for (size_t i = 0; i < size;) {		
		size_t next_i = i + addend;
		if (next_i > size)
			next_i = size;
		size_t avail_in = next_i - i;
		i = next_i;
		BrotliDecoderResult result = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
		while (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
			size_t avail_out = kBufferSize;
			uint8_t* next_out = buffer;			
			result = BrotliDecoderDecompressStream(
				state, &avail_in, &next_in, &avail_out, &next_out, &total_out);
			if (total_out > total_out_limit)
				break;			
			strDst.append((char*)buffer, kBufferSize - avail_out);			

		}
		if (total_out > total_out_limit)
			break;
		if (result != BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT)
			break;
	}

	BrotliDecoderDestroyInstance(state);
	free(buffer);
	return true;
}

bool CBrotliMng::brotliEncode(string & strDst, string & strSrc)
{
	strDst.clear();
	BrotliEncoderState* s = BrotliEncoderCreateInstance(NULL, NULL, NULL);
	if (!s)
		return false;
	
	int quality = 4;
	int lgwin = 22;
	BrotliEncoderSetParameter(s, BROTLI_PARAM_QUALITY, quality);
	BrotliEncoderSetParameter(s, BROTLI_PARAM_LGWIN, lgwin);

	uint8_t* input_buf = (uint8_t*)&strSrc[0];
	int out_len = 1024;
	uint8_t* out_buf = (uint8_t*)malloc(out_len);
	if (!out_buf) {
		return false;
	}
	size_t size = static_cast<size_t>(strSrc.size());
	size_t available_in = size;			/* length of real data */
			 /* length of buf befor call compress, and length of remain size of buf after compress */
	const uint8_t* next_in = input_buf;  /* start of in buff */
	BROTLI_BOOL is_eof = BROTLI_FALSE;
	int MAX_LOOP_NUM = 1000000;
	int loop = 0;
	size_t total_size = 0;
	do {
		size_t available_out = out_len;
		uint8_t* next_out = out_buf;
		if (0 == available_in) {
			is_eof = BROTLI_TRUE;
		}
		if (!BrotliEncoderCompressStream(s,
			is_eof ? BROTLI_OPERATION_FINISH : BROTLI_OPERATION_PROCESS,
			&available_in, &next_in, &available_out, &next_out, &total_size))
		{
			break;
		}
		strDst.append((char*)out_buf, out_len - available_out);
		size -= (out_len - available_out);
		

		if (BrotliEncoderIsFinished(s)) {
			break;
		}	
	} while (++loop < MAX_LOOP_NUM);
	if (loop >= MAX_LOOP_NUM) {
		LOG_ERROR("****** error to happed ");
	}

	BrotliEncoderDestroyInstance(s);
	free(out_buf);
	return true;
}


bool CBrotliMng::brotliDecode(string & strDst, string & strSrc)
{
	size_t size = static_cast<int>(strSrc.size());
	size_t available_in = size;
	char *pBuffer_in = &strSrc[0];
	const uint8_t* next_in = (const uint8_t*)pBuffer_in;
	strDst.clear();
	size_t out_len = size * 10;
	size_t len = out_len;
	string & strTemp = strDst;
	strTemp.resize(out_len);
	uint8_t* next_out = (uint8_t*)&strTemp[0];
	BrotliDecoderResult ret = BrotliDecoderDecompress(available_in, next_in, &out_len, next_out);
	if (ret != BrotliDecoderResult::BROTLI_DECODER_RESULT_SUCCESS) {
		//LOG_ERROR("(%d)out_len =%d size=%d %d ", len, out_len, size, (int)ret);
	}
	if (len > out_len) {		
		strDst.erase(out_len, len - out_len);		
	}
	//LOG_INFO("(%d)out_len =%d size%d %d ", len, out_len, size, (int)ret);
	
	return true;
}