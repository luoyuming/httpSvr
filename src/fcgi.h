#ifndef FCGI_H
#define FCGI_H
#include "common.h"
#include "fastcgi.h"

typedef struct
{
    SOCKET sockfd_;
    int requestId_; 
    int flag_; 

} FastCgi_t;

void FastCgi_init(FastCgi_t *c);
void FastCgi_finit(FastCgi_t *c);
void setRequestId(FastCgi_t *c, int requestId);
void makeHeader(FCGI_Header *ptrHead, int type, int request,
                       int contentLength, int paddingLength);
FCGI_BeginRequestBody makeBeginRequestBody(int role, int keepConnection);
int makeNameValueBody(const char *name, int nameLen,const char *value, int valueLen,
                      unsigned char *bodyBuffPtr, int *bodyLen);

bool startConnect(FastCgi_t *c, const char *ip, int port);
int sendStartRequestRecord(FastCgi_t *c);
int sendParams(FastCgi_t *c, const char *name, const char *value);
int sendEndRequestRecord(FastCgi_t *c);
bool readFromPhp(FastCgi_t *c, string & strResp);
char *findStartHtml(char *content);
void getHtmlFromContent(FastCgi_t *c, char *content);
int cgi_write(int fd, void *buffer, int length);
int cgi_read(int fd, void *buffer, int length);

#endif
