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

ld_cond_t LDi_initcond = LD_COND_INIT;
ld_mutex_t LDi_initcondmtx;

static LDClient *theClient;
ld_rwlock_t LDi_clientlock = LD_RWLOCK_INIT;

void (*LDi_statuscallback)(int);

void
LDi_earlyinit(void)
{
    LDi_mtxinit(&LDi_allocmtx);
    LDi_mtxinit(&LDi_initcondmtx);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    LDi_initializerng();

    theClient = LDAlloc(sizeof(*theClient));
    if (!theClient) {
        LDi_log(LD_LOG_CRITICAL, "no memory for the client\n");
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
LDConfigNew(const char *const mobileKey)
{
    LD_ASSERT(mobileKey);

    LDi_once(&LDi_earlyonce, LDi_earlyinit);

    LDConfig *const config = LDAlloc(sizeof(*config)); LD_ASSERT(config);

    memset(config, 0, sizeof(*config));

    LD_ASSERT(LDSetString(&config->appURI, "https://app.launchdarkly.com"));

    LD_ASSERT(LDSetString(&config->eventsURI, "https://mobile.launchdarkly.com"));

    LD_ASSERT(LDSetString(&config->mobileKey, mobileKey));

    LD_ASSERT(LDSetString(&config->streamURI, "https://clientstream.launchdarkly.com"));

    config->allAttributesPrivate = false;
    config->backgroundPollingIntervalMillis = 3600000;
    config->connectionTimeoutMillis = 10000;
    config->disableBackgroundUpdating = false;
    config->eventsCapacity = 100;
    config->eventsFlushIntervalMillis = 30000;
    config->offline = false;
    config->pollingIntervalMillis = 300000;
    config->privateAttributeNames = NULL;
    config->streaming = true;
    config->useReport = false;

    return config;
}

void LDConfigSetAllAttributesPrivate(LDConfig *const config, const bool private)
{
    LD_ASSERT(config); config->allAttributesPrivate = private;
}

void LDConfigSetBackgroundPollingIntervalMillis(LDConfig *const config, const int millis)
{
    LD_ASSERT(config); config->backgroundPollingIntervalMillis = millis;
}

void LDConfigSetAppURI(LDConfig *const config, const char *const uri)
{
    LD_ASSERT(config); LD_ASSERT(uri); LD_ASSERT(LDSetString(&config->appURI, uri));
}

void LDConfigSetConnectionTimeoutMillies(LDConfig *const config, const int millis)
{
    LD_ASSERT(config); config->connectionTimeoutMillis = millis;
}

void LDConfigSetDisableBackgroundUpdating(LDConfig *const config, const bool disable)
{
    LD_ASSERT(config); config->disableBackgroundUpdating = disable;
}

void LDConfigSetEventsCapacity(LDConfig *const config, const int capacity)
{
    LD_ASSERT(config); config->eventsCapacity = capacity;
}

void LDConfigSetEventsFlushIntervalMillis(LDConfig *const config, const int millis)
{
    LD_ASSERT(config); config->eventsFlushIntervalMillis = millis;
}

void LDConfigSetEventsURI(LDConfig *const config, const char *const uri)
{
    LD_ASSERT(config); LD_ASSERT(uri); LD_ASSERT(LDSetString(&config->eventsURI, uri));
}

void LDConfigSetMobileKey(LDConfig *const config, const char *const key)
{
    LD_ASSERT(config); LD_ASSERT(key); LD_ASSERT(LDSetString(&config->mobileKey, key));
}

void LDConfigSetOffline(LDConfig *const config, const bool offline)
{
    LD_ASSERT(config); config->offline = offline;
}

void LDConfigSetStreaming(LDConfig *const config, const bool streaming)
{
    LD_ASSERT(config); config->streaming = streaming;
}

void LDConfigSetPollingIntervalMillis(LDConfig *const config, const int millis)
{
    LD_ASSERT(config); config->pollingIntervalMillis = millis;
}

void LDConfigSetStreamURI(LDConfig *const config, const char *const uri)
{
    LD_ASSERT(config); LD_ASSERT(uri); LD_ASSERT(LDSetString(&config->streamURI, uri));
}

void LDConfigSetUseReport(LDConfig *const config, const bool report)
{
    LD_ASSERT(config); config->useReport = report;
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
LDClientInit(LDConfig *const config, LDUser *const user, const unsigned int maxwaitmilli)
{
    LD_ASSERT(config); LD_ASSERT(user);

    LDi_once(&LDi_earlyonce, LDi_earlyinit);

    checkconfig(config);

    LDi_initevents(config->eventsCapacity);

    LDi_wrlock(&LDi_clientlock);

    if (!theClient) {
        LDi_wrunlock(&LDi_clientlock);
        return NULL;
    }

    LDClient *const client = theClient;

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
    client->background = false;
    client->isinit = false;
    client->allFlags = NULL;

    char *const flags = LDi_loaddata("features", user->key);
    if (flags) {
        LDi_clientsetflags(client, false, flags, 1);
        LDFree(flags);
    }

    LDi_recordidentify(client, user);
    LDi_wrunlock(&LDi_clientlock);

    LDi_once(&LDi_threadsonce, threadsinit);

    if (maxwaitmilli) {
        LDClientAwaitInitialized(client, maxwaitmilli);
    }

    return client;
}

LDClient *
LDClientGet()
{
    return theClient;
}

void
LDClientSetOffline(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_wrlock(&LDi_clientlock);
    client->offline = true;
    LDi_wrunlock(&LDi_clientlock);
}

void
LDClientSetOnline(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_wrlock(&LDi_clientlock);
    client->offline = false;
    client->isinit = false;
    LDi_wrunlock(&LDi_clientlock);
}

bool
LDClientIsOffline(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_rdlock(&LDi_clientlock);
    bool offline = client->offline;
    LDi_rdunlock(&LDi_clientlock);
    return offline;
}

void
LDClientSetBackground(LDClient *const client, const bool background)
{
    LD_ASSERT(client);
    LDi_wrlock(&LDi_clientlock);
    client->background = background;
    LDi_startstopstreaming(background);
    LDi_wrunlock(&LDi_clientlock);
}

void
LDClientIdentify(LDClient *const client, LDUser *const user)
{
    LD_ASSERT(client); LD_ASSERT(user);
    LDi_wrlock(&LDi_clientlock);
    if (user != client->user) {
        LDi_freeuser(client->user);
    }
    client->user = user;
    client->allFlags = NULL;
    LDi_updatestatus(client, 0);
    char *const flags = LDi_loaddata("features", client->user->key);
    if (flags) {
        LDi_clientsetflags(client, false, flags, 1);
        LDFree(flags);
    }
    LDi_reinitializeconnection();
    LDi_recordidentify(client, user);
    LDi_wrunlock(&LDi_clientlock);
}

void
LDClientClose(LDClient *const client)
{
    LD_ASSERT(client);

    LDi_wrlock(&LDi_clientlock);
    client->dead = true;
    LDNode *const oldhash = client->allFlags;
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
LDClientGetLockedFlags(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_rdlock(&LDi_clientlock);
    LDNode *const flags = client->allFlags;
    LDi_rdunlock(&LDi_clientlock);
    return flags;
}

bool
LDClientIsInitialized(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_rdlock(&LDi_clientlock);
    bool isinit = client->isinit;
    LDi_rdunlock(&LDi_clientlock);
    return isinit;
}

bool
LDClientAwaitInitialized(LDClient *const client, const unsigned int timeoutmilli)
{
    LD_ASSERT(client);
    LDi_mtxenter(&LDi_initcondmtx);
    LDi_rdlock(&LDi_clientlock);
    if (client->isinit) {
        LDi_rdunlock(&LDi_clientlock);
        LDi_mtxleave(&LDi_initcondmtx);
        return true;
    }
    LDi_rdunlock(&LDi_clientlock);

    LDi_condwait(&LDi_initcond, &LDi_initcondmtx, timeoutmilli);
    LDi_mtxleave(&LDi_initcondmtx);

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
LDClientSaveFlags(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_rdlock(&LDi_clientlock);
    char *const serialized = LDi_hashtostring(client->allFlags, true);
    LDi_rdunlock(&LDi_clientlock);
    return serialized;
}

void
LDClientRestoreFlags(LDClient *const client, const char *const data)
{
    LD_ASSERT(client); LD_ASSERT(data);
    if (LDi_clientsetflags(client, true, data, 1)) {
        LDi_savedata("features", client->user->key, data);
    }
}

bool
LDi_clientsetflags(LDClient *const client, const bool needlock, const char *const data, const int flavor)
{
    LD_ASSERT(client); LD_ASSERT(data);

    cJSON *const payload = cJSON_Parse(data);

    if (!payload) {
        LDi_log(LD_LOG_ERROR, "LDi_clientsetflags parsing failed\n");
        return false;
    }

    LDNode *hash = NULL;
    if (payload->type == cJSON_Object) {
        hash = LDi_jsontohash(payload, flavor);
    }
    cJSON_Delete(payload);

    if (needlock) {
        LDi_wrlock(&LDi_clientlock);
    }

    bool statuschange = client->isinit == false;
    LDNode *const oldhash = client->allFlags;
    client->allFlags = hash;

    /* tell application we are ready to go */
    if (statuschange) {
        LDi_updatestatus(client, 1);
    }

    if (needlock) {
        LDi_wrunlock(&LDi_clientlock);
    }

    LDi_freehash(oldhash);

    return true;
}

void
LDi_savehash(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_rdlock(&LDi_clientlock);
    char *const serialized = LDi_hashtostring(client->allFlags, true);
    LDi_savedata("features", client->user->key, serialized);
    LDi_rdunlock(&LDi_clientlock);
    LDFree(serialized);
}

/*
 * a block of functions to look up feature flags
 */

bool
LDBoolVariation(LDClient *const client, const char *const key, const bool fallback)
{
    LD_ASSERT(client); LD_ASSERT(key);

    bool result;
    LDi_rdlock(&LDi_clientlock);

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeBool) {
        result = node->b;
    } else {
        result = fallback;
    }

    LDi_recordfeature(client, client->user, node, key, LDNodeBool,
        (double)result, NULL, NULL, (double)fallback, NULL, NULL);

    LDi_rdunlock(&LDi_clientlock);
    return result;
}

int
LDIntVariation(LDClient *const client, const char *const key, const int fallback)
{
    LD_ASSERT(client); LD_ASSERT(key);

    int result;
    LDi_rdlock(&LDi_clientlock);

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeNumber) {
        result = (int)node->n;
    } else {
        result = fallback;
    }

    LDi_recordfeature(client, client->user, node, key, LDNodeNumber,
        (double)result, NULL, NULL, (double)fallback, NULL, NULL);

    LDi_rdunlock(&LDi_clientlock);

    return result;
}

double
LDDoubleVariation(LDClient *const client, const char *const key, const double fallback)
{
    LD_ASSERT(client); LD_ASSERT(key);

    double result;
    LDi_rdlock(&LDi_clientlock);

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeNumber) {
        result = node->n;
    } else {
        result = fallback;
    }

    LDi_recordfeature(client, client->user, node, key, LDNodeNumber,
        result, NULL, NULL, fallback, NULL, NULL);

    LDi_rdunlock(&LDi_clientlock);

    return result;
}

char *
LDStringVariation(LDClient *const client, const char *const key,
    const char *const fallback, char *const buffer, const size_t space)
{
    LD_ASSERT(client); LD_ASSERT(key); LD_ASSERT(!(!buffer && space));

    const char *result;
    LDi_rdlock(&LDi_clientlock);

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeString) {
        result = node->s;
    } else {
        result = fallback;
    }

    size_t len = strlen(result);
    if (len > space - 1) {
        len = space - 1;
    }
    memcpy(buffer, result, len);
    buffer[len] = 0;

    LDi_recordfeature(client, client->user, node, key, LDNodeString,
        0.0, result, NULL, 0.0, fallback, NULL);

    LDi_rdunlock(&LDi_clientlock);

    return buffer;
}

char *
LDStringVariationAlloc(LDClient *const client, const char *const key, const char *const fallback)
{
    LD_ASSERT(client); LD_ASSERT(key);

    const char *value;
    LDi_rdlock(&LDi_clientlock);

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeString) {
        value = node->s;
    } else {
        value = fallback;
    }

    char *const result = LDi_strdup(value);

    LDi_recordfeature(client, client->user, node, key, LDNodeString,
        0.0, result, NULL, 0.0, fallback, NULL);

    LDi_rdunlock(&LDi_clientlock);

    return result;
}

LDNode *
LDJSONVariation(LDClient *const client, const char *const key, LDNode *const fallback)
{
    LD_ASSERT(client); LD_ASSERT(key);

    LDNode *result;
    LDi_rdlock(&LDi_clientlock);

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeHash) {
        result = node->h;
    } else {
        result = fallback;
    }

    LDi_recordfeature(client, client->user, node, key, LDNodeHash,
        0.0, NULL, result, 0.0, NULL, fallback);

    LDi_rdunlock(&LDi_clientlock);

    return result;
}

void
LDJSONRelease(LDNode *const node)
{
    LDi_freehash(node);
}

void
LDClientTrack(LDClient *const client, const char *const name)
{
    LD_ASSERT(client); LD_ASSERT(name);
    LDi_rdlock(&LDi_clientlock);
    LDi_recordtrack(client, client->user, name, NULL);
    LDi_rdunlock(&LDi_clientlock);
}

void
LDClientTrackData(LDClient *const client, const char *const name, LDNode *const data)
{
    LD_ASSERT(client); LD_ASSERT(name); LD_ASSERT(data);
    LDi_rdlock(&LDi_clientlock);
    LDi_recordtrack(client, client->user, name, data);
    LDi_rdunlock(&LDi_clientlock);
}

void
LDClientFlush(LDClient *const client)
{
    LD_ASSERT(client); LDi_condsignal(&LDi_bgeventcond);
}

void
LDClientRegisterFeatureFlagListener(LDClient *const client, const char *const key, LDlistenerfn fn)
{
    LD_ASSERT(client != NULL); LD_ASSERT(key != NULL);

    struct listener *const list = LDAlloc(sizeof(*list)); LD_ASSERT(list);

    list->fn = fn;
    list->key = LDi_strdup(key);
    LD_ASSERT(list->key);

    LDi_wrlock(&LDi_clientlock);
    list->next = client->listeners;
    client->listeners = list;
    LDi_wrunlock(&LDi_clientlock);
}

bool
LDClientUnregisterFeatureFlagListener(LDClient *const client, const char *const key, LDlistenerfn fn)
{
    LD_ASSERT(client); LD_ASSERT(key);

    struct listener *list = NULL, *prev = NULL;

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
LDConfigAddPrivateAttribute(LDConfig *const config, const char *const key)
{
    LD_ASSERT(config); LD_ASSERT(key);

    LDNode *const node = LDAlloc(sizeof(*node)); LD_ASSERT(node);
    memset(node, 0, sizeof(*node));

    node->key = LDi_strdup(key);
    LD_ASSERT(node->key);

    node->type = LDNodeBool;
    node->b = true;

    HASH_ADD_KEYPTR(hh, config->privateAttributeNames, node->key, strlen(node->key), node);
}

void
LDi_updatestatus(struct LDClient_i *const client, const bool isinit)
{
    client->isinit = isinit;
    if (LDi_statuscallback) {
        LDi_statuscallback(isinit);
    }
    LDi_condsignal(&LDi_initcond);
};
