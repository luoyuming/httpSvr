#pragma once
#include "common.h"
#include "util.h"
#include "log.h"
#include "socketService.h"
#include "gzipCode.h"
#include "brotliMng.h"
#include "codeDeflate.h"

class CompressMng {

public:
    CompressMng();
    ~CompressMng();
    void encode(std::shared_ptr<PACKAGE_INFO> & pPkgInfo);
    void encode(PACKAGE_INFO & info);
private:
    std::shared_ptr<CBrotliMng>     m_br;
    std::shared_ptr<CGzip>          m_gzip;
    std::shared_ptr<Compression>    m_deflate;
};