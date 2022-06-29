#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include <launchdarkly/api.h>

#include "ldinternal.h"

#define LD_STREAMTIMEOUT_MS 300000
#define LD_USER_AGENT_HEADER "User-Agent: CClient/" LD_SDK_VERSION
#define UNUSED(x) (void)(x)

struct MemoryStruct
{
    char * memory;
    size_t size;
};

struct streamdata
{
    struct MemoryStruct mem;
    double              lastdatatime;
    curl_off_t          lastdataamt;
    struct LDClient *   client;
    struct LDSSEParser *parser;
};

struct cbhandlecontext
{
    struct LDClient *client;
    void (*cb)(struct LDClient *, int);
};

typedef size_t (*WriteCB)(void *, size_t, size_t, void *);

static size_t
WriteMemoryCallback(
    void *const contents, size_t size, size_t nmemb, void *const rawContext)
{
    size_t               realSize;
    struct MemoryStruct *mem;

    LD_ASSERT(rawContext);

    realSize = size * nmemb;
    mem      = (struct MemoryStruct *)rawContext;

    mem->memory = LDRealloc(mem->memory, mem->size + realSize + 1);

    if (mem->memory == NULL) {
        LD_LOG(LD_LOG_CRITICAL, "not enough memory (realloc returned NULL)");

        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realSize);
    mem->size += realSize;
    mem->memory[mem->size] = 0;

    return realSize;
}

static size_t
StreamWriteCallback(
    void *const contents, size_t size, size_t nmemb, void *const rawContext)
{
    size_t             realSize;
    struct streamdata *context;

    LD_ASSERT(rawContext);

    realSize = size * nmemb;
    context  = (struct streamdata *)rawContext;

    if (LDSSEParserProcess(context->parser, contents, realSize)) {
        return realSize;
    }

    return 0;
}

/*
 * Handle progress from CURL. If there is no increase in transferred data for
 * more than LD_STREAMTIMEOUT_MS, then return 1, which will cause
 * CURL to abort the connection. When the pending CURL operation returns,
 * then the reconnect logic activates.
 */
static int
ProgressCallback(void *clientp,
                 curl_off_t dltotal, /* Total expected download. */
                 curl_off_t dlnow,   /* Total downloaded so far. */
                 curl_off_t ultotal, /* Total expected upload. */
                 curl_off_t ulnow)   /* The total uploaded so far. */
{
    struct streamdata *context;
    double currentTime;
    double elapsedTime;

    /* These are a required part of the callback interface,
     * but we don't need them. */
    UNUSED(dltotal);
    UNUSED(ultotal);
    UNUSED(ulnow);

    LD_ASSERT(clientp);
    context  = (struct streamdata *)clientp;

    if (context->lastdataamt == dlnow) {
        LDi_getMonotonicMilliseconds(&currentTime);

        elapsedTime = currentTime - context->lastdatatime;
        if (elapsedTime > LD_STREAMTIMEOUT_MS) {
            /* Returning non-zero will cause CURL to abort the request. */
            return 1;
        }
    } else {
        LDi_getMonotonicMilliseconds(&context->lastdatatime);
        context->lastdataamt = dlnow;
    }

    return 0;
}

static curl_socket_t
SocketCallback(
    void *const                 rawContext,
    curlsocktype                socketType,
    struct curl_sockaddr *const addr)
{
    struct cbhandlecontext *ctx;
    curl_socket_t           fd;
    (void)socketType;

    LD_ASSERT(rawContext);

    fd  = socket(addr->family, addr->socktype, addr->protocol);
    ctx = (struct cbhandlecontext *const)rawContext;

    if (ctx) {
        LD_LOG(LD_LOG_TRACE, "about to call connection handle callback");
        ctx->cb(ctx->client, fd);
        LD_LOG(LD_LOG_TRACE, "finished calling connection handle callback");
    }

    return fd;
}

/* returns LDBooleanFalse on failure, results left in clean state */
static LDBoolean
prepareShared(
    const char *const            url,
    const struct LDConfig *const config,
    CURL **                      r_curl,
    struct curl_slist **         r_headers,
    WriteCB                      headercb,
    void *const                  headerdata,
    WriteCB                      datacb,
    void *const                  data,
    const struct LDClient *const client)
{

    char               headerauth[256];
    struct curl_slist *headers, *headerstmp;
    CURL *             curl;

    LD_ASSERT(url);
    LD_ASSERT(config);
    LD_ASSERT(client);

    headers    = NULL;
    headerstmp = NULL;

    if (!(curl = curl_easy_init())) {
        LD_LOG(LD_LOG_CRITICAL, "curl_easy_init returned NULL");
        goto error;
    }

    if (curl_easy_setopt(curl, CURLOPT_URL, url) != CURLE_OK) {
        LD_LOG_1(
            LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_URL failed on: %s", url);

        goto error;
    }

    if (snprintf(
            headerauth,
            sizeof(headerauth),
            "Authorization: %s",
            client->mobileKey) < 0)
    {
        LD_LOG(
            LD_LOG_CRITICAL,
            "snprintf during Authorization header creation failed");

        goto error;
    }

    if (!(headerstmp = curl_slist_append(headers, headerauth))) {
        LD_LOG(LD_LOG_CRITICAL, "curl_slist_append failed for headerauth");

        goto error;
    }
    headers = headerstmp;

    if (!(headerstmp = curl_slist_append(headers, LD_USER_AGENT_HEADER))) {
        LD_LOG(LD_LOG_CRITICAL, "curl_slist_append failed for headeragent");

        goto error;
    }
    headers = headerstmp;

    if (curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headercb) != CURLE_OK) {
        LD_LOG(
            LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_HEADERFUNCTION failed");

        goto error;
    }

    if (curl_easy_setopt(curl, CURLOPT_HEADERDATA, headerdata) != CURLE_OK) {
        LD_LOG(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_HEADERDATA failed");

        goto error;
    }

    if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, datacb) != CURLE_OK) {
        LD_LOG(
            LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_WRITEFUNCTION failed");

        goto error;
    }

    if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, data) != CURLE_OK) {
        LD_LOG(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_WRITEDATA failed");

        goto error;
    }

    if (config->proxyURI) {
        if (curl_easy_setopt(curl, CURLOPT_PROXY, config->proxyURI) != CURLE_OK)
        {
            LD_LOG(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_PROXY failed");

            goto error;
        }
    }

    if (curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, config->verifyPeer) !=
        CURLE_OK)
    {
        LD_LOG(
            LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_SSL_VERIFYPEER failed");

        goto error;
    }

    /* Added by external applications to set SSL certificate */
    if (config->certFile) {
        if (curl_easy_setopt(curl, CURLOPT_CAINFO, config->certFile) !=
            CURLE_OK) {
            LD_LOG(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_CAINFO failed");

            goto error;
        }
    }

    if (curl_easy_setopt(
            curl,
            CURLOPT_CONNECTTIMEOUT_MS,
            (long)config->connectionTimeoutMillis) != CURLE_OK)
    {
        LD_LOG(
            LD_LOG_CRITICAL,
            "curl_easy_setopt CURLOPT_CONNECTTIMEOUT_MS failed");

        goto error;
    }

    *r_curl    = curl;
    *r_headers = headers;

    return LDBooleanTrue;

error:
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    return LDBooleanFalse;
}

void
LDi_cancelread(const int handle)
{
#ifdef _WINDOWS
    shutdown(handle, SD_BOTH);
#else
    shutdown(handle, SHUT_RDWR);
#endif
}

/*
 * this function reads data and passes it to the stream callback.
 * it doesn't return except after a disconnect. (or some other failure.)
 */
void
LDi_readstream(
    struct LDClient *const    client,
    long *                     response,
    struct LDSSEParser *const parser,
    void                      cbhandle(struct LDClient *, int))
{
    CURLcode               res;
    struct MemoryStruct    headers;
    struct streamdata      streamdata;
    struct cbhandlecontext handledata;
    CURL *                 curl;
    struct curl_slist *    headerlist, *headertmp;
    struct LDJSON *        userJSON;
    char *                 userJSONText;
    char                   url[4096];

    LD_ASSERT(client);
    LD_ASSERT(response);
    LD_ASSERT(parser);
    LD_ASSERT(cbhandle);

    curl       = NULL;
    headerlist = NULL;
    headertmp  = NULL;

    handledata.client = client;
    handledata.cb     = cbhandle;

    memset(&headers, 0, sizeof(headers));
    memset(&streamdata, 0, sizeof(streamdata));

    streamdata.parser       = parser;
    streamdata.lastdataamt  = 0;
    streamdata.client       = client;

    LDi_getMonotonicMilliseconds(&streamdata.lastdatatime);

    LDi_rwlock_rdlock(&client->shared->sharedUserLock);
    LDi_rwlock_rdlock(&client->clientLock);

    userJSON = LDi_userToJSON(
        client->shared->sharedUser, LDBooleanFalse, LDBooleanFalse, NULL);

    LDi_rwlock_rdunlock(&client->clientLock);
    LDi_rwlock_rdunlock(&client->shared->sharedUserLock);

    if (userJSON == NULL) {
        LD_LOG(LD_LOG_CRITICAL, "failed to convert user to user");

        return;
    }

    userJSONText = LDJSONSerialize(userJSON);
    LDJSONFree(userJSON);

    if (userJSONText == NULL) {
        LD_LOG(LD_LOG_CRITICAL, "failed to serialize user");

        return;
    }

    if (client->shared->sharedConfig->useReport) {
        if (snprintf(
                url,
                sizeof(url),
                "%s/meval",
                client->shared->sharedConfig->streamURI) < 0)
        {
            LDFree(userJSONText);

            LD_LOG(LD_LOG_CRITICAL, "snprintf usereport failed");

            return;
        }
    } else {
        int                  status;
        size_t               b64len;
        unsigned char *const b64text = LDi_base64_encode(
            (unsigned char *)userJSONText, strlen(userJSONText), &b64len);

        if (!b64text) {
            LDFree(userJSONText);

            LD_LOG(LD_LOG_ERROR, "LDi_base64_encode == NULL in LDi_readstream");

            return;
        }

        status = snprintf(
            url,
            sizeof(url),
            "%s/meval/%s",
            client->shared->sharedConfig->streamURI,
            b64text);

        LDFree(b64text);

        if (status < 0) {
            LDFree(userJSONText);

            LD_LOG(LD_LOG_ERROR, "snprintf !usereport failed");

            return;
        }
    }

    if (client->shared->sharedConfig->useReasons) {
        const size_t len = strlen(url);

        if (snprintf(
                url + len, sizeof(url) - len, "?withReasons=true") < 0)
        {
            LDFree(userJSONText);

            LD_LOG(LD_LOG_ERROR, "snprintf useReason failed");

            return;
        }
    }

    if (!prepareShared(
            url,
            client->shared->sharedConfig,
            &curl,
            &headerlist,
            &WriteMemoryCallback,
            &headers,
            &StreamWriteCallback,
            &streamdata,
            client))
    {
        LDFree(userJSONText);

        return;
    }

    if (client->shared->sharedConfig->useReport) {
        if (curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "REPORT") != CURLE_OK)
        {
            LD_LOG(
                LD_LOG_CRITICAL,
                "curl_easy_setopt CURLOPT_CUSTOMREQUEST failed");

            goto cleanup;
        }

        if (!(headertmp = curl_slist_append(
                  headerlist, "Content-Type: application/json")))
        {
            LD_LOG(LD_LOG_CRITICAL, "curl_slist_append failed for headermime");

            goto cleanup;
        }
        headerlist = headertmp;

        if (curl_easy_setopt(curl, CURLOPT_POSTFIELDS, userJSONText) !=
            CURLE_OK) {
            LD_LOG(
                LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_POSTFIELDS failed");

            goto cleanup;
        }
    }

    if (curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, SocketCallback) !=
        CURLE_OK)
    {
        LD_LOG(
            LD_LOG_CRITICAL,
            "curl_easy_setopt CURLOPT_OPENSOCKETFUNCTION failed");

        goto cleanup;
    }

    if (curl_easy_setopt(curl, CURLOPT_OPENSOCKETDATA, &handledata) != CURLE_OK)
    {
        LD_LOG(
            LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_OPENSOCKETDATA failed");

        goto cleanup;
    }

    if (curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist) != CURLE_OK) {
        LD_LOG(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_HTTPHEADER failed");

        goto cleanup;
    }

    /* This needs set or progress callbacks will not be made. */
    if (curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0)) {
        LD_LOG(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_NOPROGRESS failed");

        goto cleanup;
    }

    /* Expose the data to the progress callback so it can track the last time
     * that data was received. */
    if (curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &streamdata) != CURLE_OK) {
        LD_LOG(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_XFERINFODATA failed");

        goto cleanup;
    }

    if (curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,
                        ProgressCallback)) {
        LD_LOG(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_XFERINFOFUNCTION failed");

        goto cleanup;
    }

    LD_LOG_1(LD_LOG_INFO, "connecting to stream %s", url);
    res = curl_easy_perform(curl);

    /* CURL_LAST = 99 so the union of curl responses + http response codes should have no overlap. */
    if (res == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        LD_LOG_1(LD_LOG_DEBUG, "curl response code %d", (int)response_code);
        *response = response_code;
    } else {
        *response = res;
    }

cleanup:
    LDFree(streamdata.mem.memory);
    LDFree(headers.memory);
    LDFree(userJSONText);

    curl_slist_free_all(headerlist);

    curl_easy_cleanup(curl);
}

char *
LDi_fetchfeaturemap(struct LDClient *const client, int *response)
{
    struct MemoryStruct headers, data;
    CURL *              curl       = NULL;
    struct curl_slist * headerlist = NULL, *headertmp = NULL;
    struct LDJSON *     userJSON;
    char *              userJSONText;
    char                url[4096];

    memset(&headers, 0, sizeof(headers));
    memset(&data, 0, sizeof(data));

    LDi_rwlock_rdlock(&client->shared->sharedUserLock);
    LDi_rwlock_rdlock(&client->clientLock);

    userJSON = LDi_userToJSON(
        client->shared->sharedUser, LDBooleanFalse, LDBooleanFalse, NULL);

    LDi_rwlock_rdunlock(&client->clientLock);
    LDi_rwlock_rdunlock(&client->shared->sharedUserLock);

    if (userJSON == NULL) {
        LD_LOG(LD_LOG_CRITICAL, "failed to convert user to user");

        return NULL;
    }

    userJSONText = LDJSONSerialize(userJSON);
    LDJSONFree(userJSON);

    if (userJSONText == NULL) {
        LD_LOG(LD_LOG_CRITICAL, "failed to serialize user");

        return NULL;
    }

    if (client->shared->sharedConfig->useReport) {
        if (snprintf(
                url,
                sizeof(url),
                "%s/msdk/evalx/user",
                client->shared->sharedConfig->appURI) < 0)
        {
            LDFree(userJSONText);

            LD_LOG(LD_LOG_CRITICAL, "snprintf usereport failed");

            return NULL;
        }
    } else {
        int                  status;
        size_t               b64len;
        unsigned char *const b64text = LDi_base64_encode(
            (unsigned char *)userJSONText, strlen(userJSONText), &b64len);

        if (!b64text) {
            LDFree(userJSONText);

            LD_LOG(
                LD_LOG_CRITICAL,
                "LDi_base64_encode == NULL in LDi_fetchfeaturemap");

            return NULL;
        }

        status = snprintf(
            url,
            sizeof(url),
            "%s/msdk/evalx/users/%s",
            client->shared->sharedConfig->appURI,
            b64text);

        LDFree(b64text);

        if (status < 0) {
            LDFree(userJSONText);

            LD_LOG(LD_LOG_ERROR, "snprintf !usereport failed");

            return NULL;
        }
    }

    if (client->shared->sharedConfig->useReasons) {
        const size_t len = strlen(url);
        if (snprintf(
                url + len, sizeof(url) - len, "?withReasons=true") < 0)
        {
            LDFree(userJSONText);

            LD_LOG(LD_LOG_ERROR, "snprintf useReason failed");

            return NULL;
        }
    }

    if (!prepareShared(
            url,
            client->shared->sharedConfig,
            &curl,
            &headerlist,
            &WriteMemoryCallback,
            &headers,
            &WriteMemoryCallback,
            &data,
            client))
    {
        LDFree(userJSONText);

        return NULL;
    }

    LDFree(userJSONText);

    if (client->shared->sharedConfig->useReport) {
        if (curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "REPORT") != CURLE_OK)
        {
            LD_LOG(
                LD_LOG_CRITICAL,
                "curl_easy_setopt CURLOPT_CUSTOMREQUEST failed");

            goto error;
        }

        if (!(headertmp = curl_slist_append(
                  headerlist, "Content-Type: application/json")))
        {
            LD_LOG(LD_LOG_CRITICAL, "curl_slist_append failed for headermime");

            goto error;
        }
        headerlist = headertmp;

        if (curl_easy_setopt(curl, CURLOPT_POSTFIELDS, userJSONText) !=
            CURLE_OK) {
            LD_LOG(
                LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_POSTFIELDS failed");

            goto error;
        }
    }

    if (curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist) != CURLE_OK) {
        LD_LOG(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_HTTPHEADER failed");
        goto error;
    }

    if (curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)client->shared->sharedConfig->requestTimeoutMillis)) {
        LD_LOG(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_TIMEOUT_MS failed");

        goto error;
    }

    if (curl_easy_perform(curl) == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        *response = response_code;
    } else {
        *response = -1;
    }

    LDFree(headers.memory);

    curl_slist_free_all(headerlist);

    curl_easy_cleanup(curl);

    return data.memory;

error:
    LDFree(data.memory);
    LDFree(headers.memory);

    curl_slist_free_all(headerlist);

    curl_easy_cleanup(curl);

    return NULL;
}

void
LDi_sendevents(
    struct LDClient *const client,
    const char *const      eventdata,
    const char *const      payloadUUID,
    int *const             response)
{
    struct MemoryStruct headers, data;
    CURL *              curl       = NULL;
    struct curl_slist * headerlist = NULL, *headertmp = NULL;
    char                url[4096];

/* This is done as a macro so that the string is a literal */
#define LD_PAYLOAD_ID_HEADER "X-LaunchDarkly-Payload-ID: "

    /* do not need to add space for null termination because of sizeof */
    char payloadIdHeader[sizeof(LD_PAYLOAD_ID_HEADER) + LD_UUID_SIZE];

    memset(&headers, 0, sizeof(headers));
    memset(&data, 0, sizeof(data));

    if (snprintf(
            url,
            sizeof(url),
            "%s/mobile",
            client->shared->sharedConfig->eventsURI) < 0)
    {
        LD_LOG(LD_LOG_CRITICAL, "snprintf config->eventsURI failed");

        return;
    }

    if (!prepareShared(
            url,
            client->shared->sharedConfig,
            &curl,
            &headerlist,
            &WriteMemoryCallback,
            &headers,
            &WriteMemoryCallback,
            &data,
            client))
    {
        return;
    }

    if (!(headertmp =
              curl_slist_append(headerlist, "Content-Type: application/json")))
    {
        LD_LOG(LD_LOG_CRITICAL, "curl_slist_append failed for headermime");

        goto cleanup;
    }
    headerlist = headertmp;

    if (!(headertmp =
              curl_slist_append(headerlist, "X-LaunchDarkly-Event-Schema: 3")))
    {
        LD_LOG(LD_LOG_CRITICAL, "curl_slist_append failed for headerschema");

        goto cleanup;
    }
    headerlist = headertmp;

    {
        int len;

        len = snprintf(
            payloadIdHeader,
            sizeof(payloadIdHeader),
            "%s%s",
            LD_PAYLOAD_ID_HEADER,
            payloadUUID);

        if (len != sizeof(payloadIdHeader) - 1) {
            LD_LOG(LD_LOG_CRITICAL, "unable to generate payload ID header");
            goto cleanup;
        }
    }

#undef LD_PAYLOAD_ID_HEADER

    if (!(headertmp = curl_slist_append(headerlist, payloadIdHeader))) {
        goto cleanup;
    }
    headerlist = headertmp;

    if (curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist) != CURLE_OK) {
        LD_LOG(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_HTTPHEADER failed");

        goto cleanup;
    }

    if (curl_easy_setopt(curl, CURLOPT_POSTFIELDS, eventdata) != CURLE_OK) {
        LD_LOG(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_POSTFIELDS failed");

        goto cleanup;
    }

    if (curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)client->shared->sharedConfig->requestTimeoutMillis)) {
        LD_LOG(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_TIMEOUT_MS failed");

        goto cleanup;
    }

    if (curl_easy_perform(curl) == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        *response = response_code;
    } else {
        *response = -1;
    }

cleanup:
    LDFree(data.memory);
    LDFree(headers.memory);

    curl_slist_free_all(headerlist);

    curl_easy_cleanup(curl);
}
