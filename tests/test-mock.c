#include <stdio.h>
#include <string.h>

#include <launchdarkly/api.h>

/* http_server must be imported before concurrency.h because of windows.h */
#include "test-utils/http_server.h"

#include "assertion.h"
#include "concurrency.h"
#include "ldinternal.h"

#ifdef _WIN32
#include <ws2tcpip.h>
#endif

static ld_socket_t acceptFD;
static int         acceptPort;

static struct LDJSON *
makeMinimalFlag(const char *const key, struct LDJSON *const value)
{
    struct LDFlag  flag;
    struct LDJSON *flagJSON;

    LD_ASSERT(key);
    LD_ASSERT(value);

    flag.key                  = (char *)key;
    flag.value                = value;
    flag.version              = 3;
    flag.flagVersion          = 4;
    flag.variation            = 2;
    flag.trackEvents          = LDBooleanFalse;
    flag.reason               = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted              = LDBooleanFalse;

    LD_ASSERT(flagJSON = LDi_flag_to_json(&flag));

    LDJSONFree(value);

    return flagJSON;
}

static struct LDJSON *
makeBasicPutBody()
{
    struct LDJSON *payload, *flag, *value;

    LD_ASSERT(payload = LDNewObject());

    LD_ASSERT(value = LDNewBool(LDBooleanTrue));
    LD_ASSERT(flag = makeMinimalFlag("flag1", value));

    LD_ASSERT(LDObjectSetKey(payload, "flag1", flag));

    return payload;
}

static void
testBasicPoll_sendResponse(ld_socket_t fd)
{
    char *         serialized;
    struct LDJSON *payload;

    LD_ASSERT(payload = makeBasicPutBody());

    LD_ASSERT(serialized = LDJSONSerialize(payload));

    LDi_send200(fd, serialized);

    LDJSONFree(payload);
    LDFree(serialized);
}

static THREAD_RETURN
testBasicPoll_thread(void *const unused)
{
    struct LDHTTPRequest request;

    LD_ASSERT(unused == NULL);

    LDHTTPRequestInit(&request);

    LDi_readHTTPRequest(acceptFD, &request);

    LD_ASSERT(strcmp("GET", request.requestMethod) == 0);
    LD_ASSERT(request.requestBody == NULL);

    LD_ASSERT(
        strcmp(
            "/msdk/evalx/users/eyJrZXkiOiJteS11c2VyIn0=", request.requestURL) ==
        0);

    LD_ASSERT(
        strcmp(
            "key",
            LDGetText(
                LDObjectLookup(request.requestHeaders, "Authorization"))) == 0);

    LD_ASSERT(
        strcmp(
            "CClient/" LD_SDK_VERSION,
            LDGetText(LDObjectLookup(request.requestHeaders, "User-Agent"))) ==
        0);

    testBasicPoll_sendResponse(request.requestSocket);

    LDHTTPRequestDestroy(&request);

    return THREAD_RETURN_DEFAULT;
}

static void
testBasicPoll()
{
    ld_thread_t      thread;
    struct LDConfig *config;
    struct LDClient *client;
    struct LDUser *  user;
    char             pollURL[1024];

    LDi_listenOnRandomPort(&acceptFD, &acceptPort);
    LDi_thread_create(&thread, testBasicPoll_thread, NULL);

    LD_ASSERT(snprintf(pollURL, 1024, "http://127.0.0.1:%d", acceptPort) > 0);

    LD_ASSERT(config = LDConfigNew("key"));
    LDConfigSetStreaming(config, LDBooleanFalse);
    LDConfigSetAppURI(config, pollURL);

    LD_ASSERT(user = LDUserNew("my-user"));
    LD_ASSERT(client = LDClientInit(config, user, 1000 * 10));

    LD_ASSERT(LDBoolVariation(client, "flag1", LDBooleanFalse));

    LDClientClose(client);
    LDi_closeSocket(acceptFD);
    LDi_thread_join(&thread);
}

static void
testBasicStream_sendResponse(ld_socket_t fd)
{
    char *         putBodySerialized;
    struct LDJSON *putBody;
    char           payload[1024];

    LD_ASSERT(putBody = makeBasicPutBody());

    LD_ASSERT(putBodySerialized = LDJSONSerialize(putBody));

    LD_ASSERT(
        snprintf(payload, 1024, "event: put\ndata: %s\n\n", putBodySerialized) >
        0);

    LDi_send200(fd, payload);

    LDJSONFree(putBody);
    LDFree(putBodySerialized);
}

static THREAD_RETURN
testBasicStream_thread(void *const unused)
{
    struct LDHTTPRequest request;

    LD_ASSERT(unused == NULL);

    LDHTTPRequestInit(&request);

    LDi_readHTTPRequest(acceptFD, &request);

    LD_ASSERT(
        strcmp("/meval/eyJrZXkiOiJteS11c2VyIn0=", request.requestURL) == 0);
    LD_ASSERT(strcmp("GET", request.requestMethod) == 0);
    LD_ASSERT(request.requestBody == NULL);

    LD_ASSERT(
        strcmp(
            "key",
            LDGetText(
                LDObjectLookup(request.requestHeaders, "Authorization"))) == 0);

    LD_ASSERT(
        strcmp(
            "CClient/" LD_SDK_VERSION,
            LDGetText(LDObjectLookup(request.requestHeaders, "User-Agent"))) ==
        0);

    testBasicStream_sendResponse(request.requestSocket);

    LDHTTPRequestDestroy(&request);

    return THREAD_RETURN_DEFAULT;
}

static void
testBasicStream()
{
    ld_thread_t      thread;
    struct LDConfig *config;
    struct LDClient *client;
    struct LDUser *  user;
    char             streamURL[1024];

    LDi_listenOnRandomPort(&acceptFD, &acceptPort);
    LDi_thread_create(&thread, testBasicStream_thread, NULL);

    LD_ASSERT(snprintf(streamURL, 1024, "http://127.0.0.1:%d", acceptPort) > 0);

    LD_ASSERT(config = LDConfigNew("key"));
    LDConfigSetStreamURI(config, streamURL);

    LD_ASSERT(user = LDUserNew("my-user"));
    LD_ASSERT(client = LDClientInit(config, user, 1000 * 10));

    LD_ASSERT(LDBoolVariation(client, "flag1", LDBooleanFalse));

    LDClientClose(client);
    LDi_closeSocket(acceptFD);
    LDi_thread_join(&thread);
}

int
main()
{
#ifdef _WIN32
    WSADATA wsaData;

    LD_ASSERT(WSAStartup(0x202, &wsaData) == 0);
#endif

    LDBasicLoggerThreadSafeInitialize();
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLoggerThreadSafe);

    testBasicPoll();
    testBasicStream();

#ifdef _WIN32
    WSACleanup();
#endif

    LDBasicLoggerThreadSafeShutdown();

    return 0;
}