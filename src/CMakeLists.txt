
PROJECT(httpSvr)
cmake_minimum_required(VERSION 3.1)
FILE(REMOVE build)
FILE(MAKE_DIRECTORY build)
FILE(MAKE_DIRECTORY build/log)

ADD_DEFINITIONS(-DLINUX_OS -DHTTP_V2)


# include directories
INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}/include
  /usr/local/include
  /usr/include
  /usr/include/openssl
  ${PROJECT_SOURCE_DIR}/brotli/include   
  ${PROJECT_SOURCE_DIR}/json
  ${PROJECT_SOURCE_DIR}/tinyxml
  ./
)

# lib directories
LINK_DIRECTORIES(
  /usr/local/lib
  /usr/lib
  /usr/lib/x86_64-linux-gnu  
)

FILE(GLOB_RECURSE XML_FILES  ${CMAKE_SOURCE_DIR}/tinyxml/*.cpp)
FILE(GLOB_RECURSE BROTLI_FILES  ${CMAKE_SOURCE_DIR}/brotli/*.c)
SET (ALL_FILES  ${BROTLI_FILES} ${XML_FILES}) 
list(APPEND BASE_SDK m glog pthread iconv crypto z ssl rt)
SET (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -ggdb3 -std=c99 ")
SET (CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS} -Wall -g -ggdb3 -std=c++14")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -std=c++14")
SET(CMAKE_C_FLAGS_RELEASE  "$ENV{CXXFLAGS} -std=c99")
AUX_SOURCE_DIRECTORY(. DIR_SRCS)
ADD_EXECUTABLE(${PROJECT_NAME} ${DIR_SRCS} ${ALL_FILES})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${BASE_SDK})


