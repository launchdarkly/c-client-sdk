#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "ldapi.h"
#include "ldinternal.h"


struct MemoryStruct {
    char *memory;
    size_t size;
};
struct streamdata {
    int (*callback)(const char *);
    struct MemoryStruct mem;
};
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        /* out of memory! */ 
        printf("not enough memory (realloc returned NULL)\n");
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
        printf("not enough memory (realloc returned NULL)\n");
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
            streamdata->callback(mem->memory + eaten);
            eaten = nl - mem->memory + 1;
            nl = memchr(mem->memory + eaten, '\n', mem->size - eaten);
        }
        memmove(mem->memory, mem->memory + eaten, eaten);
        mem->size -= eaten;
    }
 
    return realsize;

}

static char *
fetch_url(const char *url, const char *authkey, int *response)
{
    char auth[256];
    CURL *curl;
    int res;
    long response_code;
    struct curl_slist *chunk = NULL;
    struct MemoryStruct headers, data;

    memset(&headers, 0, sizeof(headers));
    memset(&data, 0, sizeof(data));

    curl = curl_easy_init();

    snprintf(auth, sizeof(auth), "Authorization: %s", authkey);
    chunk = curl_slist_append(chunk, auth);
    chunk = curl_slist_append(chunk, "User-Agent: CClient/0.1");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&data);

    res = curl_easy_perform(curl);
    if (res == 0) {
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        *response = response_code;
    } else {
        *response = -1;
    }

    free(headers.memory);

    curl_easy_cleanup(curl);

    return data.memory;

}

static char *
post_data(const char *url, const char *authkey, const char *postbody, int *response)
{
    char auth[256];
    CURL *curl;
    int res;
    long response_code;
    struct curl_slist *chunk = NULL;
    struct MemoryStruct headers, data;

    memset(&headers, 0, sizeof(headers));
    memset(&data, 0, sizeof(data));

    curl = curl_easy_init();

    snprintf(auth, sizeof(auth), "Authorization: %s", authkey);
    chunk = curl_slist_append(chunk, auth);
    chunk = curl_slist_append(chunk, "User-Agent: CClient/0.1");
    chunk = curl_slist_append(chunk, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&data);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postbody);

    res = curl_easy_perform(curl);
    if (res == 0) {
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        *response = response_code;
    } else {
        *response = -1;
    }

    free(headers.memory);

    curl_easy_cleanup(curl);

    return data.memory;
}

/*
 * this function reads data and passes it to the stream callback.
 * it doesn't return except after a disconnect. (or some other failure.)
 */
void
LDi_readstream(const char *url, const char *authkey, int *response, int callback(const char *))
{
    char auth[256];
    CURL *curl;
    int res;
    long response_code;
    struct curl_slist *chunk = NULL;
    struct MemoryStruct headers;
    struct streamdata streamdata;

    memset(&headers, 0, sizeof(headers));
    memset(&streamdata, 0, sizeof(streamdata));
    streamdata.callback = callback;

    curl = curl_easy_init();

    snprintf(auth, sizeof(auth), "Authorization: %s", authkey);
    chunk = curl_slist_append(chunk, auth);
    chunk = curl_slist_append(chunk, "User-Agent: CClient/0.1");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StreamWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&streamdata);

    printf("connecting to stream %s\n", url);
    res = curl_easy_perform(curl);
    printf("curl res %d\n", res);
    if (res == 0) {
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        printf("curl response code %d\n", (int)response_code);
        *response = response_code;
    } else {
        *response = -1;
    }

    free(headers.memory);

    curl_easy_cleanup(curl);

    free(streamdata.mem.memory);

}


LDMapNode *
LDi_fetchfeaturemap(LDClient *client, int *response)
{
    char *userurl = LDi_usertourl(client->user);

    char url[4096];
    snprintf(url, sizeof(url), "%s/msdk/eval/users/%s", client->config->appURI, userurl);
    free(userurl);

    char *data = fetch_url(url, client->config->mobileKey, response);
    if (!data)
        return NULL;
    cJSON *payload = cJSON_Parse(data);
    free(data);

    LDMapNode *hash = NULL;
    if (payload->type == cJSON_Object) {
        hash = LDi_jsontohash(payload, 0);
    }
    cJSON_Delete(payload);

    return hash;
}

void
LDi_sendevents(const char *url, const char *authkey, const char *eventdata, int *response)
{
    char *data = post_data(url, authkey, eventdata, response);
    free(data);
}
