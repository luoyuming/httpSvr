#pragma once
#include "common.h"

#ifdef WIN_OS 

#define mkdir(x, y)         _mkdir(x)
#define vsnprintf           _vsnprintf
#endif