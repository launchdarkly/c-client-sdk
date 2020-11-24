#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <ws2tcpip.h>
    #include <windows.h>
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <unistd.h>
#endif

#include <launchdarkly/memory.h>

#include "http_parser.h"

#include "assertion.h"

#include "test-utils/http_server.h"

static void
LDi_writeAll(const ld_socket_t socket, const char *const buffer,
    const size_t bufferSize)
{
    size_t sent;

    LD_ASSERT(buffer);

    sent = 0;

    while (sent < bufferSize) {
        int status;

        status = send(socket, buffer + sent, bufferSize - sent, 0);
        LD_ASSERT(status > 0);

        sent += status;
    }
}

static void
LDi_writeAllString(const ld_socket_t socket, const char *const string)
{
    LDi_writeAll(socket, string, strlen(string));
}

void
LDi_send200(const ld_socket_t socket, const char *const body)
{
    LDi_writeAllString(socket, "HTTP/1.1 200 OK\r\n");
    LDi_writeAllString(socket, "Connection: Closed\r\n");

    if (body != NULL) {
        char contentSizeHeader[1024];

        snprintf(contentSizeHeader, 1024, "Content-Length: %d\r\n",
            (int)strlen(body));

        LDi_writeAllString(socket, contentSizeHeader);
    }

    LDi_writeAllString(socket, "\r\n");

    if (body != NULL) {
        LDi_writeAllString(socket, body);
    }
}

void
LDHTTPRequestInit(struct LDHTTPRequest *const request)
{
    LD_ASSERT(request);

    request->done            = false;
    request->requestURL      = NULL;
    request->requestMethod   = NULL;
    request->requestBody     = NULL;
    request->requestSocket   = -1;
    request->lastHeaderField = NULL;

    request->requestHeaders = LDNewObject();
    LD_ASSERT(request->requestHeaders);
}

void
LDHTTPRequestDestroy(struct LDHTTPRequest *const request)
{
    if (request) {
        LDFree(request->requestURL);
        LDFree(request->requestMethod);
        LDFree(request->requestBody);
        LDFree(request->lastHeaderField);
        LDJSONFree(request->requestHeaders);

        if (request->requestSocket != -1) {
            LDi_closeSocket(request->requestSocket);
        }
    }
}

void
LDi_closeSocket(const ld_socket_t socket)
{
    #ifdef _WIN32
        closesocket(socket);
    #else
        close(socket);
    #endif
}

void
LDi_listenOnRandomPort(ld_socket_t *const resultSocket,
    int *const resultPort)
{
    ld_socket_t acceptFD;
    int status;
    struct sockaddr_in serverAddress;
    socklen_t serverAddressSize;

    LD_ASSERT(resultSocket);

    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family      = AF_INET;
    serverAddress.sin_port        = 0; /* random port */
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    serverAddressSize = sizeof(serverAddress);

    acceptFD = socket(PF_INET, SOCK_STREAM, 0);
    LD_ASSERT(acceptFD != -1);

    status = bind(acceptFD, (struct sockaddr *)&serverAddress,
        sizeof(serverAddress));
    LD_ASSERT(status >= 0);

    status = listen(acceptFD, SOMAXCONN);
    LD_ASSERT(status >= 0);

    status = getsockname(acceptFD, (struct sockaddr *)&serverAddress,
        &serverAddressSize);
    LD_ASSERT(status >= 0);

    *resultPort   = ntohs(serverAddress.sin_port);
    *resultSocket = acceptFD;
}

static int
LDi_onURL(http_parser *const parser, const char *const url, const size_t length)
{
    struct LDHTTPRequest *request;

    LD_ASSERT(parser);
    LD_ASSERT(url);

    request = (struct LDHTTPRequest *)parser->data;
    LD_ASSERT(request);

    request->requestURL = LDStrNDup(url, length);
    LD_ASSERT(request->requestURL);

    return 0;
}

static int
LDi_onBody(http_parser *const parser, const char *const body,
    const size_t length)
{
    struct LDHTTPRequest *request;

    LD_ASSERT(parser);
    LD_ASSERT(body);

    request = (struct LDHTTPRequest *)parser->data;
    LD_ASSERT(request);

    request->requestBody = LDStrNDup(body, length);
    LD_ASSERT(request->requestBody);

    return 0;
}

static int
LDi_onHeaderField(http_parser *const parser, const char *const field,
    const size_t length)
{
    struct LDHTTPRequest *request;

    LD_ASSERT(parser);
    LD_ASSERT(field);

    request = (struct LDHTTPRequest *)parser->data;
    LD_ASSERT(request);

    LD_ASSERT(request->lastHeaderField == NULL);
    request->lastHeaderField = LDStrNDup(field, length);
    LD_ASSERT(request->lastHeaderField);

    return 0;
}

static int
LDi_onHeaderValue(http_parser *const parser, const char *const value,
    const size_t length)
{
    char *valueCopy;
    struct LDJSON *tmp;
    struct LDHTTPRequest *request;

    LD_ASSERT(parser);
    LD_ASSERT(value);

    request = (struct LDHTTPRequest *)parser->data;
    LD_ASSERT(request);

    LD_ASSERT(request->lastHeaderField);

    LD_ASSERT(valueCopy = LDStrNDup(value, length));
    LD_ASSERT(tmp = LDNewText(valueCopy));
    LDFree(valueCopy);

    LD_ASSERT(LDObjectSetKey(request->requestHeaders,
        request->lastHeaderField, tmp));

    LDFree(request->lastHeaderField);
    request->lastHeaderField = NULL;

    return 0;
}

static int
LDi_onMessageComplete(http_parser *const parser)
{
    struct LDHTTPRequest *request;

    LD_ASSERT(parser);

    request = (struct LDHTTPRequest *)parser->data;
    LD_ASSERT(request);

    request->done = true;

    return 0;
}

void
LDi_readHTTPRequest(const ld_socket_t acceptFD,
    struct LDHTTPRequest *const request)
{
    ld_socket_t clientFD;
    struct sockaddr_in clientAddress;
    socklen_t clientAddressSize;
    http_parser parser;
    http_parser_settings settings;
    char buffer[4096];
    int readSize;

    LD_ASSERT(request);

    http_parser_init(&parser, HTTP_REQUEST);
    http_parser_settings_init(&settings);

    clientAddressSize            = sizeof(clientAddress);
    settings.on_url              = LDi_onURL;
    settings.on_message_complete = LDi_onMessageComplete;
    settings.on_body             = LDi_onBody;
    settings.on_header_field     = LDi_onHeaderField;
    settings.on_header_value     = LDi_onHeaderValue;
    parser.data                  = (void *)request;

    clientFD = accept(acceptFD, (struct sockaddr *)&clientAddress,
        &clientAddressSize);
    LD_ASSERT(clientFD >= 0);

    while (!request->done) {
        readSize = recv(clientFD, &buffer, 4096, 0);
        LD_ASSERT(readSize >= 0);
        http_parser_execute(&parser, &settings, buffer, readSize);
    }

    request->requestMethod = LDStrDup(http_method_str(parser.method));
    LD_ASSERT(request->requestMethod);
    request->requestSocket = clientFD;
}
