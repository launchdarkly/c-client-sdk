#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "curl/curl.h"

#include "ldapi.h"
#include "ldinternal.h"

#define LD_STREAMTIMEOUT 300

struct MemoryStruct {
    char *memory;
    size_t size;
};

struct streamdata {
    int (*callback)(LDClient *client, const char *);
    struct MemoryStruct mem;
    time_t lastdatatime;
    double lastdataamt;
    LDClient *client;
};

struct cbhandlecontext {
    LDClient *client;
    void (*cb)(LDClient *, int);
};

typedef size_t (*WriteCB)(void*, size_t, size_t, void*);

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        /* out of memory! */
        LDi_log(LD_LOG_CRITICAL, "not enough memory (realloc returned NULL)");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

static size_t
StreamWriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct streamdata *streamdata = userp;
    struct MemoryStruct *mem = &streamdata->mem;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        /* out of memory! */
        LDi_log(LD_LOG_CRITICAL, "not enough memory (realloc returned NULL)");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    char *nl;
    nl = memchr(mem->memory, '\n', mem->size);
    if (nl) {
        size_t eaten = 0;
        while (nl) {
            *nl = 0;
            streamdata->callback(streamdata->client, mem->memory + eaten);
            eaten = nl - mem->memory + 1;
            nl = memchr(mem->memory + eaten, '\n', mem->size - eaten);
        }
        mem->size -= eaten;
        memmove(mem->memory, mem->memory + eaten, mem->size);
    }
    return realsize;
}

static curl_socket_t
SocketCallback(void *const c, curlsocktype type, struct curl_sockaddr *const addr)
{
    struct cbhandlecontext *const ctx = c;
    const curl_socket_t fd = socket(addr->family, addr->socktype, addr->protocol);
    if (ctx) {
        LDi_log(LD_LOG_TRACE, "about to call connection handle callback");
        ctx->cb(ctx->client, fd);
        LDi_log(LD_LOG_TRACE, "finished calling connection handle callback");
    }
    return fd;
};

/* returns false on failure, results left in clean state */
static bool
prepareShared(const char *const url, const char *const authkey, CURL **r_curl, struct curl_slist **r_headers,
    WriteCB headercb, void *const headerdata, WriteCB datacb, void *const data)
{
    CURL *const curl = curl_easy_init();

    if (!curl) {
        LDi_log(LD_LOG_CRITICAL, "curl_easy_init returned NULL"); goto error;
    }

    if (curl_easy_setopt(curl, CURLOPT_URL, url) != CURLE_OK) {
        LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_URL failed on: %s", url); goto error;
    }

    char headerauth[256];
    if (snprintf(headerauth, sizeof(headerauth), "Authorization: %s", authkey) < 0) {
        LDi_log(LD_LOG_CRITICAL, "snprintf during Authorization header creation failed"); goto error;
    }

    struct curl_slist *headers = NULL;
    if (!(headers = curl_slist_append(headers, headerauth))) {
        LDi_log(LD_LOG_CRITICAL, "curl_slist_append failed for headerauth"); goto error;
    }

    const char *const headeragent = "User-Agent: CClient/0.1";
    if (!(headers = curl_slist_append(headers, headeragent))) {
        LDi_log(LD_LOG_CRITICAL, "curl_slist_append failed for headeragent"); goto error;
    }

    if (curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headercb) != CURLE_OK) {
        LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_HEADERFUNCTION failed"); goto error;
    }

    if (curl_easy_setopt(curl, CURLOPT_HEADERDATA, headerdata) != CURLE_OK) {
        LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_HEADERDATA failed"); goto error;
    }

    if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, datacb) != CURLE_OK) {
        LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_WRITEFUNCTION failed"); goto error;
    }

    if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, data) != CURLE_OK) {
        LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_WRITEDATA failed"); goto error;
    }

    *r_curl = curl; *r_headers = headers;

    return true;

  error:
    if (curl) { curl_easy_cleanup(curl); }

    return false;
}

/*
 * record the timestamp of the last received data. if nothing has been
 * seen for a while, disconnect. this shouldn't normally happen.
 */
static int
progressinspector(void *const v, double dltotal, double dlnow, double ultotal, double ulnow)
{
    struct streamdata *const streamdata = v;
    const time_t now = time(NULL);

    if (streamdata->lastdataamt != dlnow) {
        streamdata->lastdataamt = dlnow;
        streamdata->lastdatatime = now;
    }

    if (now - streamdata->lastdatatime > LD_STREAMTIMEOUT) {
        LDi_log(LD_LOG_ERROR, "giving up stream, too slow");
        return 1;
    }

    LDi_rdlock(&streamdata->client->clientLock);
    const bool failed = streamdata->client->status == LDStatusFailed;
    const bool stopping = streamdata->client->status == LDStatusShuttingdown;
    const bool offline = streamdata->client->offline;
    LDi_rdunlock(&streamdata->client->clientLock);

    if ( failed || stopping || offline ) {
        return 1;
    }

    return 0;
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
LDi_readstream(LDClient *const client, int *response, int cbdata(LDClient *, const char *), void cbhandle(LDClient *, int))
{
    struct MemoryStruct headers; struct streamdata streamdata; struct cbhandlecontext handledata;
    CURL *curl = NULL; struct curl_slist *headerlist = NULL;

    handledata.client = client; handledata.cb = cbhandle;

    memset(&headers, 0, sizeof(headers)); memset(&streamdata, 0, sizeof(streamdata));

    streamdata.callback = cbdata; streamdata.lastdatatime = time(NULL); streamdata.client = client;

    LDi_rdlock(&client->clientLock);
    char *const jsonuser = LDi_usertojsontext(client, client->user, false);
    if (!jsonuser) {
        LDi_rdunlock(&client->clientLock);
        LDi_log(LD_LOG_CRITICAL, "cJSON_PrintUnformatted == NULL in LDi_readstream failed");
        return;
    }
    LDi_rdunlock(&client->clientLock);

    char url[4096];
    if (client->config->useReport) {
        if (snprintf(url, sizeof(url), "%s/meval", client->config->streamURI) < 0) {
            free(jsonuser);
            LDi_log(LD_LOG_CRITICAL, "snprintf usereport failed"); return;
        }
    }
    else {
        size_t b64len;
        unsigned char *const b64text = LDi_base64_encode((unsigned char*)jsonuser, strlen(jsonuser), &b64len);

        if (!b64text) {
            free(jsonuser);
            LDi_log(LD_LOG_ERROR, "LDi_base64_encode == NULL in LDi_readstream"); return;
        }

        const int status = snprintf(url, sizeof(url), "%s/meval/%s", client->config->streamURI, b64text);
        free(b64text);

        if (status < 0) {
            free(jsonuser);
            LDi_log(LD_LOG_ERROR, "snprintf !usereport failed"); return;
        }
    }

    if (!prepareShared(url, client->config->mobileKey, &curl, &headerlist, &WriteMemoryCallback, &headers, &StreamWriteCallback, &streamdata)) {
        free(jsonuser);
        return;
    }

    free(jsonuser);

    if (client->config->useReport) {
        if (curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "REPORT") != CURLE_OK) {
            LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_CUSTOMREQUEST failed");
            curl_easy_cleanup(curl); return;
        }

        const char* const headermime = "Content-Type: application/json";
        if (!(headerlist = curl_slist_append(headerlist, headermime))) {
            LDi_log(LD_LOG_CRITICAL, "curl_slist_append failed for headermime");
            curl_easy_cleanup(curl); return;
        }

        if (curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonuser) != CURLE_OK) {
            LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_POSTFIELDS failed");
            curl_easy_cleanup(curl); return;
        }
    }

    if (curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, SocketCallback) != CURLE_OK) {
        LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_OPENSOCKETFUNCTION failed");
        curl_easy_cleanup(curl); return;
    }

    if (curl_easy_setopt(curl, CURLOPT_OPENSOCKETDATA, &handledata) != CURLE_OK) {
        LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_OPENSOCKETDATA failed");
        curl_easy_cleanup(curl); return;
    }

    if (curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist) != CURLE_OK) {
        LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_HTTPHEADER failed");
        curl_easy_cleanup(curl); return;
    }

    if (curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0) != CURLE_OK) {
        LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_NOPROGRESS failed");
        curl_easy_cleanup(curl); return;
    }

    if (curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressinspector) != CURLE_OK) {
        LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_PROGRESSFUNCTION failed");
        curl_easy_cleanup(curl); return;
    }

    if (curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, (void *)&streamdata) != CURLE_OK) {
        LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_PROGRESSDATA failed");
        curl_easy_cleanup(curl); return;
    }

    LDi_log(LD_LOG_INFO, "connecting to stream %s", url);
    const CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        LDi_log(LD_LOG_DEBUG, "curl response code %d", (int)response_code);
        *response = response_code;
    }
    else if (res == CURLE_PARTIAL_FILE) {
        *response = -2;
    }
    else {
        *response = -1;
    }

    free(streamdata.mem.memory); free(headers.memory);

    curl_easy_cleanup(curl);
}

char *
LDi_fetchfeaturemap(LDClient *const client, int *response)
{
    struct MemoryStruct headers, data;
    CURL *curl = NULL; struct curl_slist *headerlist = NULL;

    memset(&headers, 0, sizeof(headers)); memset(&data, 0, sizeof(data));

    LDi_rdlock(&client->clientLock);
    char *const jsonuser = LDi_usertojsontext(client, client->user, false);
    if (!jsonuser) {
        LDi_rdunlock(&client->clientLock);
        LDi_log(LD_LOG_CRITICAL, "cJSON_PrintUnformatted == NULL in LDi_readstream failed");
        return NULL;
    }
    LDi_rdunlock(&client->clientLock);

    char url[4096];
    if (client->config->useReport) {
        if (snprintf(url, sizeof(url), "%s/msdk/evalx/user", client->config->appURI) < 0) {
            free(jsonuser);
            LDi_log(LD_LOG_CRITICAL, "snprintf usereport failed"); return NULL;
        }
    }
    else {
        size_t b64len;
        unsigned char *const b64text = LDi_base64_encode((unsigned char*)jsonuser, strlen(jsonuser), &b64len);

        if (!b64text) {
            free(jsonuser);
            LDi_log(LD_LOG_CRITICAL, "LDi_base64_encode == NULL in LDi_fetchfeaturemap"); return NULL;
        }

        const int status = snprintf(url, sizeof(url), "%s/msdk/evalx/users/%s", client->config->appURI, b64text);
        free(b64text);

        if (status < 0) {
            free(jsonuser);
            LDi_log(LD_LOG_ERROR, "snprintf !usereport failed"); return NULL;
        }
    }

    if (!prepareShared(url, client->config->mobileKey, &curl, &headerlist, &WriteMemoryCallback, &headers, &WriteMemoryCallback, &data)) {
        free(jsonuser);
        return NULL;
    }

    free(jsonuser);

    if (client->config->useReport) {
        if (curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "REPORT") != CURLE_OK) {
            LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_CUSTOMREQUEST failed");
            curl_easy_cleanup(curl); return NULL;
        }

        const char *const headermime = "Content-Type: application/json";
        if (!(headerlist = curl_slist_append(headerlist, headermime))) {
            LDi_log(LD_LOG_CRITICAL, "curl_slist_append failed for headermime");
            curl_easy_cleanup(curl); return NULL;
        }

        if (curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonuser) != CURLE_OK) {
            LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_POSTFIELDS failed");
            curl_easy_cleanup(curl); return NULL;
        }
    }

    if (curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist) != CURLE_OK) {
        LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_HTTPHEADER failed");
        curl_easy_cleanup(curl); return NULL;
    }

    if (curl_easy_perform(curl) == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        *response = response_code;
    } else {
        *response = -1;
    }

    free(headers.memory);

    curl_easy_cleanup(curl);

    return data.memory;
}

void
LDi_sendevents(LDClient *const client, const char *eventdata, int *response)
{
    struct MemoryStruct headers, data;
    CURL *curl = NULL; struct curl_slist *headerlist = NULL;

    memset(&headers, 0, sizeof(headers)); memset(&data, 0, sizeof(data));

    char url[4096];
    if (snprintf(url, sizeof(url), "%s/mobile", client->config->eventsURI) < 0) {
        LDi_log(LD_LOG_CRITICAL, "snprintf config->eventsURI failed");
        return;
    }

    if (!prepareShared(url, client->config->mobileKey, &curl, &headerlist, &WriteMemoryCallback, &headers, &WriteMemoryCallback, &data)) {
        return;
    }

    const char *const headermime = "Content-Type: application/json";
    if (!(headerlist = curl_slist_append(headerlist, headermime))) {
        LDi_log(LD_LOG_CRITICAL, "curl_slist_append failed for headermime");
        curl_easy_cleanup(curl); return;
    }

    const char *const headerschema = "X-LaunchDarkly-Event-Schema: 3";
    if (!(headerlist = curl_slist_append(headerlist, headerschema))) {
        LDi_log(LD_LOG_CRITICAL, "curl_slist_append failed for headerschema");
        curl_easy_cleanup(curl); return;
    }

    if (curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist) != CURLE_OK) {
        LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_HTTPHEADER failed");
        curl_easy_cleanup(curl); return;
    }

    if (curl_easy_setopt(curl, CURLOPT_POSTFIELDS, eventdata) != CURLE_OK) {
        LDi_log(LD_LOG_CRITICAL, "curl_easy_setopt CURLOPT_POSTFIELDS failed");
        curl_easy_cleanup(curl); return;
    }

    if (curl_easy_perform(curl) == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        *response = response_code;
    } else {
        *response = -1;
    }

    free(data.memory); free(headers.memory);

    curl_easy_cleanup(curl);
}
