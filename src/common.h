#pragma once

#include <stdio.h>
#include <math.h>
#include <atomic>
#include <stdlib.h>
#include <limits.h>
#include <memory>
#include <list>
#include <map>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <set>
#include <vector>
#include <functional>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ratio>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>
#include <sstream>
#include <stdarg.h>
#include <stdint.h>
#include <cctype>
#include <algorithm>
#include <iterator>
#include <assert.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <string.h>
#include <regex>
#include <limits.h>
#include <locale>        
#include <malloc.h>


#ifdef  LINUX_OS 
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h> 
#include <sys/resource.h>
#include <sys/epoll.h>
#include <unistd.h>  
#include <pthread.h>  
#include <signal.h> 
#include <netinet/tcp.h>
#include <netdb.h>
#include <iconv.h>
#include <aio.h>

#define  HANDLE_NULL            0
typedef  int				    HANDLE;
typedef  int				    SOCKET;
#define  INVALID_SOCKET			(SOCKET)(~0)
#define  closesocket            close
#define  CloseHandle            close         
static const int MAXEVENT = 10;
#endif


#ifdef WIN_OS 
#pragma warning (disable:4200)

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include  <codecvt>  
#include  <direct.h>
#include  <WinSock2.h>
#include  <ws2tcpip.h>
#include  <mswsock.h> 
#include  <windows.h>


#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "win_lib/libglog.lib")
#pragma comment(lib, "win_lib/libcrypto32MD.lib")
#pragma comment(lib, "win_lib/libssl32MD.lib")
#pragma comment(lib, "win_lib/libcrypto.lib")
#pragma comment(lib, "win_lib/libssl.lib")
#pragma comment(lib, "win_lib/zlib.lib")

#define  HANDLE_NULL            nullptr

#endif

using namespace std;
using namespace std::chrono;
using namespace std::placeholders;

static int const DATA_BUFSIZE = 4096;
static const int MAX_SOCKET_WAIT_MS = 50;
static const int MAX_IDLE_SEC = 20;
static const string HTTP_SVR_VERSION = "version 2.0.0";
static const int MAX_ASYN_WAIT_MS = 100;

#define LOG_OUT_PRINT 1