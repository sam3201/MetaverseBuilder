#include "protocol.h"

ProtocolMethod stringToMethod(const char *method) {
    if (strcmp(method, "GET") == 0) return PROTOCOL_GET;
    if (strcmp(method, "POST") == 0) return PROTOCOL_POST;
    return -1;
}

const char* methodToString(ProtocolMethod method) {
    switch (method) {
        case PROTOCOL_GET: return "GET";
        case PROTOCOL_POST: return "POST";
        default: return "UNKNOWN";
    }
}

ProtocolRequest* parseRequest(const char *rawRequest) {
    ProtocolRequest *request = (ProtocolRequest *)malloc(sizeof(ProtocolRequest));
    if (!request) {
        fprintf(stderr, "Failed to allocate memory for request\n");
        return NULL;
    }

    char method[10], path[256];
    sscanf(rawRequest, "%s %s", method, path);

    request->method = stringToMethod(method);
    request->path = strdup(path);
    request->body = strstr(rawRequest, "\r\n\r\n");
    if (request->body) {
        request->body += 4;
        request->body = strdup(request->body);
    } else {
        request->body = NULL;
    }

    return request;
}

char* formatResponse(const ProtocolResponse *response) {
    char *rawResponse = (char *)malloc(1024);
    if (!rawResponse) {
        fprintf(stderr, "Failed to allocate memory for response\n");
        return NULL;
    }

    snprintf(rawResponse, 1024, "HTTP/1.1 %d\r\nContent-Length: %zu\r\n\r\n%s",
             response->statusCode, strlen(response->body), response->body);

    return rawResponse;
}

ProtocolResponse* handleRequest(const ProtocolRequest *request) {
    ProtocolResponse *response = (ProtocolResponse *)malloc(sizeof(ProtocolResponse));
    if (!response) {
        fprintf(stderr, "Failed to allocate memory for response\n");
        return NULL;
    }

    if (request->method == PROTOCOL_GET && strcmp(request->path, "/") == 0) {
        response->statusCode = 200;
        response->body = strdup("Welcome to our custom protocol server!");
    } else if (request->method == PROTOCOL_POST && strcmp(request->path, "/post") == 0) {
        response->statusCode = 200;
        response->body = strdup("Post request received!");
    } else {
        response->statusCode = 404;
        response->body = strdup("Not Found");
    }

    return response;
}
