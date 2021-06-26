#pragma once

#include <launchdarkly/boolean.h>
#include <launchdarkly/json.h>

#ifdef _WIN32
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include <winsock2.h>
#endif

#ifdef _WIN32
    #define ld_socket_t SOCKET
#else
    #define ld_socket_t int
#endif

struct LDHTTPRequest {
    LDBoolean done;
    char *requestURL;
    char *requestMethod;
    char *requestBody;
    /* object */
    struct LDJSON *requestHeaders;
    ld_socket_t requestSocket;
    char *lastHeaderField;
};

void LDHTTPRequestInit(struct LDHTTPRequest *const request);
void LDHTTPRequestDestroy(struct LDHTTPRequest *const request);

void LDi_closeSocket(const ld_socket_t socket);

void LDi_listenOnRandomPort(ld_socket_t *const resultSocket,
    int *const resultPort);

void LDi_readHTTPRequest(const ld_socket_t acceptFD,
    struct LDHTTPRequest *const request);

void LDi_send200(const ld_socket_t socket, const char *const body);
