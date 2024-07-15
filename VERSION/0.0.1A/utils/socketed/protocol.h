#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    PROTOCOL_GET,
    PROTOCOL_POST
} ProtocolMethod;

typedef struct {
    ProtocolMethod method;
    char *path;
    char *body;
} ProtocolRequest;

typedef struct {
    int statusCode;
    char *body;
} ProtocolResponse;

ProtocolRequest* parseRequest(const char *rawRequest);
char* formatResponse(const ProtocolResponse *response);
ProtocolResponse* handleRequest(const ProtocolRequest *request);

#endif
