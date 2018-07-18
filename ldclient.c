#include <stdlib.h>
#include <stdio.h>
#ifndef _WINDOWS
#include <unistd.h>
#endif
#include <math.h>

#include "curl/curl.h"

#include "ldapi.h"
#include "ldinternal.h"


ld_once_t LDi_earlyonce = LD_ONCE_INIT;
ld_once_t LDi_threadsonce = LD_ONCE_INIT;


static LDClient *theClient;
ld_rwlock_t LDi_clientlock = LD_RWLOCK_INIT;

void (*LDi_statuscallback)(int);


void
LDi_earlyinit(void)
{
    LDi_mtxinit(&LDi_allocmtx);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    theClient = LDAlloc(sizeof(*theClient));
    if (!theClient) {
        LDi_log(2, "no memory for the client\n");
        return;
    }
    memset(theClient, 0, sizeof(*theClient));
}

static void
threadsinit(void)
{
    LDi_startthreads(theClient);
}

LDConfig *
LDConfigNew(const char *mobileKey)
{
    LDi_once(&LDi_earlyonce, LDi_earlyinit);

    LDConfig *config;

    config = LDAlloc(sizeof(*config));
    if (!config) {
        LDi_log(2, "no memory for config\n");
        return NULL;
    }
    memset(config, 0, sizeof(*config));
    config->allAttributesPrivate = false;
    config->backgroundPollingIntervalMillis = 3600000;
    LDSetString(&config->appURI, "https://app.launchdarkly.com");
    config->connectionTimeoutMillis = 10000;
    config->disableBackgroundUpdating = false;
    config->eventsCapacity = 100;
    config->eventsFlushIntervalMillis = 30000;
    LDSetString(&config->eventsURI, "https://mobile.launchdarkly.com");
    LDSetString(&config->mobileKey, mobileKey);
    config->offline = false;
    config->pollingIntervalMillis = 300000;
    config->privateAttributeNames = NULL;
    config->streaming = true;
    LDSetString(&config->streamURI, "https://clientstream.launchdarkly.com");
    config->useReport = false;

    return config;
}

void LDConfigSetAllAttributesPrivate(LDConfig *config, bool private)
{
    config->allAttributesPrivate = private;
}
void LDConfigSetBackgroundPollingIntervalMillis(LDConfig *config, int millis)
{
    config->backgroundPollingIntervalMillis = millis;
}
void LDConfigSetAppURI(LDConfig *config, const char *uri)
{
    LDSetString(&config->appURI, uri);
}
void LDConfigSetConnectionTimeoutMillies(LDConfig *config, int millis)
{
    config->connectionTimeoutMillis = millis;
}
void LDConfigSetDisableBackgroundUpdating(LDConfig *config, bool disable)
{
    config->disableBackgroundUpdating = disable;
}
void LDConfigSetEventsCapacity(LDConfig *config, int capacity)
{
    config->eventsCapacity = capacity;
}
void LDConfigSetEventsFlushIntervalMillis(LDConfig *config, int millis)
{
    config->eventsFlushIntervalMillis = millis;
}
void LDConfigSetEventsURI(LDConfig *config, const char *uri)
{
    LDSetString(&config->eventsURI, uri);
}
void LDConfigSetMobileKey(LDConfig *config, const char *key)
{
    LDSetString(&config->mobileKey, key);
}
void LDConfigSetOffline(LDConfig *config, bool offline)
{
    config->offline = offline;
}
void LDConfigSetStreaming(LDConfig *config, bool streaming)
{
    config->streaming = streaming;
}
void LDConfigSetPollingIntervalMillis(LDConfig *config, int millis)
{
    config->pollingIntervalMillis = millis;
}
void LDConfigSetStreamURI(LDConfig *config, const char *uri)
{
    LDSetString(&config->streamURI, uri);
}
void LDConfigSetUseReport(LDConfig *config, bool report)
{
    config->useReport = report;
}

static void
freeconfig(LDConfig *config)
{
    if (!config)
        return;
    LDFree(config->appURI);
    LDFree(config->eventsURI);
    LDFree(config->mobileKey);
    LDFree(config->streamURI);
    LDi_freehash(config->privateAttributeNames);
    LDFree(config);
}

static void
checkconfig(LDConfig *config)
{
    if (config->pollingIntervalMillis < 300000)
        config->pollingIntervalMillis = 300000;

}

LDClient *
LDClientInit(LDConfig *config, LDUser *user)
{
    LDi_once(&LDi_earlyonce, LDi_earlyinit);

    LDi_log(10, "LDClientInit called\n");

    checkconfig(config);

    LDi_initevents(config->eventsCapacity);

    LDi_wrlock(&LDi_clientlock);

    if (!theClient) {
        return NULL;
    }

    LDClient *client = theClient;

    if (client->config != config) {
        freeconfig(client->config);
    }
    client->config = config;
    if (client->user != user) {
        LDi_freeuser(client->user);
    }
    client->user = user;
    client->dead = false;
    client->offline = config->offline;

    client->isinit = false;
    client->allFlags = NULL;
    char *flags = LDi_loaddata("features", user->key);
    if (flags) {
        LDi_clientsetflags(client, false, flags, 1);
        LDFree(flags);
    }
    
    LDi_wrunlock(&LDi_clientlock);

    LDi_recordidentify(user);

    LDi_log(10, "Client init done\n");
    LDi_once(&LDi_threadsonce, threadsinit);

    return client;
}

LDClient *
LDClientGet()
{
    return theClient;
}

void
LDClientSetOffline(LDClient *client)
{
    LDi_wrlock(&LDi_clientlock);
    client->offline = true;
    LDi_wrunlock(&LDi_clientlock);
}

void
LDClientSetOnline(LDClient *client)
{
    LDi_wrlock(&LDi_clientlock);
    client->offline = false;
    client->isinit = false;
    LDi_wrunlock(&LDi_clientlock);
}

bool
LDClientIsOffline(LDClient *client)
{
    LDi_rdlock(&LDi_clientlock);
    bool offline = client->offline;
    LDi_rdunlock(&LDi_clientlock);
    return offline;
}

void
LDClientIdentify(LDClient *client, LDUser *user)
{
    LDi_wrlock(&LDi_clientlock);
    if (user != client->user) {
        LDi_freeuser(client->user);
    }
    client->user = user;
    LDi_wrunlock(&LDi_clientlock);
    LDi_recordidentify(user);
}

void
LDClientClose(LDClient *client)
{
    LDNode *oldhash;

    LDi_wrlock(&LDi_clientlock);
    client->dead = true;
    oldhash = client->allFlags;
    client->allFlags = NULL;
    freeconfig(client->config);
    client->config = NULL;
    LDi_freeuser(client->user);
    client->user = NULL;
    LDi_wrunlock(&LDi_clientlock);

    LDi_freehash(oldhash);

    /* stop the threads */
}

LDNode *
LDClientGetLockedFlags(LDClient *client)
{
    LDi_rdlock(&LDi_clientlock);
    return client->allFlags;
}

void
LDClientPutLockedFlags(LDClient *client, LDNode *flags)
{
    LDi_rdunlock(&LDi_clientlock);
}

bool
LDClientIsInitialized(LDClient *client)
{
    LDi_rdlock(&LDi_clientlock);
    bool isinit = client->isinit;
    LDi_rdunlock(&LDi_clientlock);
    return isinit;
}

void
LDSetClientStatusCallback(void (callback)(int))
{
    LDi_statuscallback = callback;
}

/*
 * save and restore flags
 */
char *
LDClientSaveFlags(LDClient *client)
{
    LDi_rdlock(&LDi_clientlock);
    char *s = LDi_hashtostring(client->allFlags, true);
    LDi_rdunlock(&LDi_clientlock);
    return s;
}

void
LDClientRestoreFlags(LDClient *client, const char *data)
{
    if (LDi_clientsetflags(client, true, data, 1)) {
        LDi_savedata("features", client->user->key, data);
    }
}

bool
LDi_clientsetflags(LDClient *client, bool needlock, const char *data, int flavor)
{
    cJSON *payload = cJSON_Parse(data);

    if (!payload) {
        LDi_log(5, "parsing failed\n");
        return false;
    }
    LDNode *hash = NULL;
    if (payload->type == cJSON_Object) {
        hash = LDi_jsontohash(payload, flavor);
    }
    cJSON_Delete(payload);

    if (needlock)
        LDi_wrlock(&LDi_clientlock);
    bool statuschange = client->isinit == false;
    LDNode *oldhash = client->allFlags;
    client->allFlags = hash;
    client->isinit = true;
    if (needlock)
        LDi_wrunlock(&LDi_clientlock);

    /* tell application we are ready to go */
    if (statuschange && LDi_statuscallback)
        LDi_statuscallback(1);

    LDi_freehash(oldhash);

    return true;
}

void
LDi_savehash(LDClient *client)
{
    LDi_rdlock(&LDi_clientlock);
    char *s = LDi_hashtostring(client->allFlags, true);
    LDi_savedata("features", client->user->key, s);
    LDi_rdunlock(&LDi_clientlock);
    LDFree(s);
}

/*
 * a block of functions to look up feature flags
 */

 bool
 isPrivateAttr(LDClient *client, const char *key)
 {
     return (LDNodeLookup(client->config->privateAttributeNames, key) != NULL) ||
        (LDNodeLookup(client->user->privateAttributeNames, key) != NULL);
 }

bool
LDBoolVariation(LDClient *client, const char *key, bool fallback)
{
    LDNode *res;
    bool b;

    LDi_rdlock(&LDi_clientlock);
    res = LDNodeLookup(client->allFlags, key);
    if (res && res->type == LDNodeBool) {
        LDi_log(15, "found result for %s\n", key);
        b = res->b;
    } else {
        LDi_log(15, "no result for %s\n", key);
        b = fallback;
    }
    if (!isPrivateAttr(client, key))
        LDi_recordfeature(client->user, key, LDNodeBool,
            (double)b, NULL, NULL, (double)fallback, NULL, NULL);
    LDi_rdunlock(&LDi_clientlock);
    return b;
}

int
LDIntVariation(LDClient *client, const char *key, int fallback)
{
    LDNode *res;
    int i;

    LDi_rdlock(&LDi_clientlock);
    res = LDNodeLookup(client->allFlags, key);
    if (res && res->type == LDNodeNumber)
        i = (int)res->n;
    else
        i = fallback;
    if (!isPrivateAttr(client, key))
        LDi_recordfeature(client->user, key, LDNodeNumber,
            (double)i, NULL, NULL, (double)fallback, NULL, NULL);
    LDi_rdunlock(&LDi_clientlock);
    return i;
}

double
LDDoubleVariation(LDClient *client, const char *key, double fallback)
{
    LDNode *res;
    double d;

    LDi_rdlock(&LDi_clientlock);
    res = LDNodeLookup(client->allFlags, key);
    if (res && res->type == LDNodeNumber)
        d = res->n;
    else
        d = fallback;
    if (!isPrivateAttr(client, key))
        LDi_recordfeature(client->user, key, LDNodeNumber,
            d, NULL, NULL, fallback, NULL, NULL);
    LDi_rdunlock(&LDi_clientlock);
    return d;
}

char *
LDStringVariation(LDClient *client, const char *key, const char *fallback,
    char *buffer, size_t space)
{
    LDNode *res;
    const char *s;
    size_t len;

    LDi_rdlock(&LDi_clientlock);
    res = LDNodeLookup(client->allFlags, key);
    if (res && res->type == LDNodeString)
        s = res->s;
    else
        s = fallback;
    
    len = strlen(s);
    if (len > space - 1)
        len = space - 1;
    memcpy(buffer, s, len);
    buffer[len] = 0;
    if (!isPrivateAttr(client, key))
        LDi_recordfeature(client->user, key, LDNodeString,
            0.0, buffer, NULL, 0.0, fallback, NULL);
    LDi_rdunlock(&LDi_clientlock);
    return buffer;
}

char *
LDStringVariationAlloc(LDClient *client, const char *key, const char *fallback)
{
    LDNode *res;
    const char *s;
    char *news;

    LDi_rdlock(&LDi_clientlock);
    res = LDNodeLookup(client->allFlags, key);
    if (res && res->type == LDNodeString)
        s = res->s;
    else
        s = fallback;
    
    news = LDi_strdup(s);
    if (!isPrivateAttr(client, key))
        LDi_recordfeature(client->user, key, LDNodeString,
            0.0, news, NULL, 0.0, fallback, NULL);
    LDi_rdunlock(&LDi_clientlock);
    return news;
}

LDNode *
LDJSONVariation(LDClient *client, const char *key, LDNode *fallback)
{
    LDNode *res;
    LDNode *j;

    LDi_rdlock(&LDi_clientlock);
    res = LDNodeLookup(client->allFlags, key);
    if (res && res->type == LDNodeHash) {
        j = res->h;
    } else {
        j = fallback;
    }
    if (!isPrivateAttr(client, key))
        LDi_recordfeature(client->user, key, LDNodeHash,
            0.0, NULL, j, 0.0, NULL, fallback);
    return j;
}

void
LDJSONRelease(LDNode *m)
{
    LDi_rdunlock(&LDi_clientlock);
}

void
LDClientTrack(LDClient *client, const char *name)
{
    LDi_rdlock(&LDi_clientlock);
    LDi_recordtrack(client->user, name, NULL);
    LDi_rdunlock(&LDi_clientlock);
}

void
LDClientTrackData(LDClient *client, const char *name, LDNode *data)
{
    LDi_rdlock(&LDi_clientlock);
    LDi_recordtrack(client->user, name, data);
    LDi_rdunlock(&LDi_clientlock);
}

void
LDClientFlush(LDClient *client)
{
    LDi_condsignal(&LDi_bgeventcond);
}

bool
LDClientRegisterFeatureFlagListener(LDClient *client, const char *key, LDlistenerfn fn)
{
    struct listener *list;

    list = LDAlloc(sizeof(*list));
    if (!list)
        return false;
    list->fn = fn;
    list->key = LDi_strdup(key);
    if (!list->key) {
        LDFree(list);
        return false;
    }

    LDi_wrlock(&LDi_clientlock);
    list->next = client->listeners;
    client->listeners = list;
    LDi_wrunlock(&LDi_clientlock);

    return true;
}

bool
LDClientUnregisterFeatureFlagListener(LDClient *client, const char *key, LDlistenerfn fn)
{
    struct listener *list, *prev;

    prev = NULL;
    LDi_wrlock(&LDi_clientlock);
    for (list = client->listeners; list; prev = list, list = list->next) {
        if (list->fn == fn && strcmp(key, list->key)) {
            if (prev) {
                prev->next = list->next;
            } else {
                client->listeners = list->next;
            }
            LDFree(list->key);
            LDFree(list);
            break;
        }
    }
    LDi_wrunlock(&LDi_clientlock);
    return list != NULL;
}

void
LDConfigAddPrivateAttribute(LDConfig *config, const char *key)
{
    LDNode *node = LDAlloc(sizeof(*node));
    memset(node, 0, sizeof(*node));
    node->key = LDi_strdup(key);
    node->type = LDNodeBool;
    node->b = true;

    HASH_ADD_KEYPTR(hh, config->privateAttributeNames, node->key, strlen(node->key), node);
}
