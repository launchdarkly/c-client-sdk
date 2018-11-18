#include <stdlib.h>
#include <stdio.h>
#ifndef _WINDOWS
#include <unistd.h>
#endif
#include <math.h>

#include "curl/curl.h"

#include "ldapi.h"
#include "ldinternal.h"

static LDClient *globalClient = NULL;

ld_once_t LDi_earlyonce = LD_ONCE_INIT;

void (*LDi_statuscallback)(int);

void
LDi_earlyinit(void)
{
    LDi_mtxinit(&LDi_allocmtx);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    LDi_initializerng();
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
    config->proxyURI = NULL;

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

void LDConfigSetProxyURI(LDConfig *const config, const char *const uri)
{
    LD_ASSERT(config); LD_ASSERT(uri); LD_ASSERT(LDSetString(&config->proxyURI, uri));
}

static void
freeconfig(LDConfig *const config)
{
    if (!config) { return; }
    LDFree(config->appURI);
    LDFree(config->eventsURI);
    LDFree(config->mobileKey);
    LDFree(config->streamURI);
    LDFree(config->proxyURI);
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
LDClientGet()
{
    return globalClient;
};

LDClient *
LDi_clientinitisolated(LDConfig *const config, LDUser *const user, const unsigned int maxwaitmilli)
{
    LD_ASSERT(config); LD_ASSERT(user);

    LDi_once(&LDi_earlyonce, LDi_earlyinit);

    checkconfig(config);

    LDClient *const client = LDAlloc(sizeof(*client));

    if (!client) {
        LDi_log(LD_LOG_CRITICAL, "no memory for the client");
        return NULL;
    }

    memset(client, 0, sizeof(*client));

    LDi_rwlockinit(&client->clientLock);

    LDi_wrlock(&client->clientLock);

    client->config = config;
    client->user = user;
    client->offline = config->offline;
    client->background = false;
    client->status = LDStatusInitializing;
    client->allFlags = NULL;
    client->threads = 3;

    client->shouldstopstreaming = false;
    client->databuffer = NULL;
    client->streamhandle = 0;

    LDi_rwlockinit(&client->eventLock);
    client->eventArray = cJSON_CreateArray();
    client->numEvents = 0;
    client->summaryEvent = LDNodeCreateHash();
    client->summaryStart = 0;

    LDi_mtxinit(&client->initCondMtx);
    LDi_condinit(&client->initCond);

    LDi_mtxinit(&client->condMtx);

    LDi_condinit(&client->eventCond);
    LDi_condinit(&client->pollCond);
    LDi_condinit(&client->streamCond);

    LDi_createthread(&client->eventThread, LDi_bgeventsender, client);
    LDi_createthread(&client->pollingThread, LDi_bgfeaturepoller, client);
    LDi_createthread(&client->streamingThread, LDi_bgfeaturestreamer, client);

    char *const flags = LDi_loaddata("features", user->key);
    if (flags) {
        LDi_clientsetflags(client, false, flags, 1);
        LDFree(flags);
    }

    LDi_recordidentify(client, user);
    LDi_wrunlock(&client->clientLock);

    if (maxwaitmilli) {
        LDClientAwaitInitialized(client, maxwaitmilli);
    }

    return client;
}

LDClient *
LDClientInit(LDConfig *const config, LDUser *const user, const unsigned int maxwaitmilli)
{
    LD_ASSERT(config); LD_ASSERT(user); LD_ASSERT(!globalClient);
    globalClient = LDi_clientinitisolated(config, user, maxwaitmilli);
    return globalClient;
}

void
LDClientSetOffline(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_wrlock(&client->clientLock);
    client->offline = true;
    LDi_wrunlock(&client->clientLock);
}

void
LDClientSetOnline(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_wrlock(&client->clientLock);
    client->offline = false;
    LDi_updatestatus(client, LDStatusInitializing);
    LDi_wrunlock(&client->clientLock);
}

bool
LDClientIsOffline(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_rdlock(&client->clientLock);
    bool offline = client->offline;
    LDi_rdunlock(&client->clientLock);
    return offline;
}

void
LDClientSetBackground(LDClient *const client, const bool background)
{
    LD_ASSERT(client);
    LDi_wrlock(&client->clientLock);
    client->background = background;
    LDi_startstopstreaming(client, background);
    LDi_wrunlock(&client->clientLock);
}

void
LDClientIdentify(LDClient *const client, LDUser *const user)
{
    LD_ASSERT(client); LD_ASSERT(user);

    LDi_wrlock(&client->clientLock);

    if (user != client->user) {
        LDi_freeuser(client->user);
    }

    client->user = user;
    client->allFlags = NULL;

    if (client->status == LDStatusInitialized) {
        LDi_updatestatus(client, LDStatusInitializing);
    }

    char *const flags = LDi_loaddata("features", client->user->key);
    if (flags) {
        LDi_clientsetflags(client, false, flags, 1);
        LDFree(flags);
    }

    LDi_reinitializeconnection(client);
    LDi_recordidentify(client, user);
    LDi_wrunlock(&client->clientLock);
}

void
LDClientClose(LDClient *const client)
{
    LD_ASSERT(client);

    LDi_wrlock(&client->clientLock);
    LDi_updatestatus(client, LDStatusShuttingdown);
    LDi_reinitializeconnection(client);
    LDi_wrunlock(&client->clientLock);

    LDi_condsignal(&client->initCond);
    LDi_condsignal(&client->eventCond);

    /* wait for threads to die */
    LDi_mtxenter(&client->initCondMtx);
    while (true) {
        LDi_wrlock(&client->clientLock);
        if (client->threads == 0) {
            LDi_updatestatus(client, LDStatusShutdown);
            LDi_wrunlock(&client->clientLock);
            break;
        }
        LDi_wrunlock(&client->clientLock);

        LDi_condwait(&client->initCond, &client->initCondMtx, 5);
    }
    LDi_mtxleave(&client->initCondMtx);

    freeconfig(client->config);
    LDi_freeuser(client->user);

    LDi_freehash(client->allFlags);

    cJSON_Delete(client->eventArray);
    /* may exist if flush failed */
    LDi_freehash(client->summaryEvent);

    LDi_rwlockdestroy(&client->clientLock);
    LDi_rwlockdestroy(&client->eventLock);

    LDi_mtxdestroy(&client->initCondMtx);
    LDi_mtxdestroy(&client->condMtx);

    LDi_conddestroy(&client->initCond);
    LDi_conddestroy(&client->eventCond);
    LDi_conddestroy(&client->pollCond);

    free(client->databuffer);

    for (struct listener *item = client->listeners; item;) {
        struct listener *const next = item->next; //must record next to make delete safe
        LDFree(item->key); LDFree(item);
        item = next;
    }

    LDFree(client);

    globalClient = NULL;
}

LDNode *
LDClientGetLockedFlags(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_rdlock(&client->clientLock);
    return client->allFlags;
}

void LDClientUnlockFlags(struct LDClient_i *const client)
{
    LD_ASSERT(client);
    LDi_rdunlock(&client->clientLock);
}

bool
LDClientIsInitialized(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_rdlock(&client->clientLock);
    bool isinit = client->status == LDStatusInitialized;
    LDi_rdunlock(&client->clientLock);
    return isinit;
}

bool
LDClientAwaitInitialized(LDClient *const client, const unsigned int timeoutmilli)
{
    LD_ASSERT(client);
    LDi_mtxenter(&client->initCondMtx);
    LDi_rdlock(&client->clientLock);
    if (client->status == LDStatusInitialized) {
        LDi_rdunlock(&client->clientLock);
        LDi_mtxleave(&client->initCondMtx);
        return true;
    }
    LDi_rdunlock(&client->clientLock);

    LDi_condwait(&client->initCond, &client->initCondMtx, timeoutmilli);
    LDi_mtxleave(&client->initCondMtx);

    LDi_rdlock(&client->clientLock);
    bool isinit = client->status == LDStatusInitialized;
    LDi_rdunlock(&client->clientLock);
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
    LDi_rdlock(&client->clientLock);
    char *const serialized = LDi_hashtostring(client->allFlags, true);
    LDi_rdunlock(&client->clientLock);
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
        LDi_log(LD_LOG_ERROR, "LDi_clientsetflags parsing failed");
        return false;
    }

    if (payload->type != cJSON_Object) {
        LDi_log(LD_LOG_ERROR, "LDi_clientsetflags did not get object");
        cJSON_Delete(payload);
        return false;
    }

    LDNode *const newhash = LDi_jsontohash(payload, flavor);

    cJSON_Delete(payload);

    if (needlock) {
        LDi_wrlock(&client->clientLock);
    }

    LDNode *const oldhash = client->allFlags;

    LDNode *oldnode, *tmp;
    HASH_ITER(hh, oldhash, oldnode, tmp) {
        LDNode *newnode = NULL;
        HASH_FIND_STR(newhash, oldnode->key, newnode);

        for (struct listener *list = client->listeners; list; list = list->next) {
            if (strcmp(list->key, oldnode->key) == 0) {
                LDi_wrunlock(&client->clientLock);
                list->fn(oldnode->key, newnode == NULL);
                LDi_wrlock(&client->clientLock);
            }
        }
    }

    client->allFlags = newhash;

    if (client->status == LDStatusInitializing) {
        LDi_updatestatus(client, LDStatusInitialized);
    }

    if (needlock) {
        LDi_wrunlock(&client->clientLock);
    }

    LDi_freehash(oldhash);

    return true;
}

void
LDi_savehash(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_rdlock(&client->clientLock);
    char *const serialized = LDi_hashtostring(client->allFlags, true);
    LDi_savedata("features", client->user->key, serialized);
    LDi_rdunlock(&client->clientLock);
    LDFree(serialized);
}

LDNode *
LDAllFlags(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_rdlock(&client->clientLock);
    LDNode *const clone = LDCloneHash(client->allFlags);
    LDi_rdunlock(&client->clientLock);
    return clone;
}

/*
 * a block of functions to look up feature flags
 */

bool
LDBoolVariation(LDClient *const client, const char *const key, const bool fallback)
{
    LD_ASSERT(client); LD_ASSERT(key);

    bool result;
    LDi_rdlock(&client->clientLock);

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeBool) {
        result = node->b;
    } else {
        result = fallback;
    }

    LDi_recordfeature(client, client->user, node, key, LDNodeBool,
        (double)result, NULL, NULL, (double)fallback, NULL, NULL);

    LDi_rdunlock(&client->clientLock);
    return result;
}

int
LDIntVariation(LDClient *const client, const char *const key, const int fallback)
{
    LD_ASSERT(client); LD_ASSERT(key);

    int result;
    LDi_rdlock(&client->clientLock);

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeNumber) {
        result = (int)node->n;
    } else {
        result = fallback;
    }

    LDi_recordfeature(client, client->user, node, key, LDNodeNumber,
        (double)result, NULL, NULL, (double)fallback, NULL, NULL);

    LDi_rdunlock(&client->clientLock);

    return result;
}

double
LDDoubleVariation(LDClient *const client, const char *const key, const double fallback)
{
    LD_ASSERT(client); LD_ASSERT(key);

    double result;
    LDi_rdlock(&client->clientLock);

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeNumber) {
        result = node->n;
    } else {
        result = fallback;
    }

    LDi_recordfeature(client, client->user, node, key, LDNodeNumber,
        result, NULL, NULL, fallback, NULL, NULL);

    LDi_rdunlock(&client->clientLock);

    return result;
}

char *
LDStringVariation(LDClient *const client, const char *const key,
    const char *const fallback, char *const buffer, const size_t space)
{
    LD_ASSERT(client); LD_ASSERT(key); LD_ASSERT(!(!buffer && space));

    const char *result;
    LDi_rdlock(&client->clientLock);

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

    LDi_rdunlock(&client->clientLock);

    return buffer;
}

char *
LDStringVariationAlloc(LDClient *const client, const char *const key, const char *const fallback)
{
    LD_ASSERT(client); LD_ASSERT(key);

    const char *value;
    LDi_rdlock(&client->clientLock);

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeString) {
        value = node->s;
    } else {
        value = fallback;
    }

    char *const result = LDi_strdup(value);

    LDi_recordfeature(client, client->user, node, key, LDNodeString,
        0.0, result, NULL, 0.0, fallback, NULL);

    LDi_rdunlock(&client->clientLock);

    return result;
}

LDNode *
LDJSONVariation(LDClient *const client, const char *const key, const LDNode *const fallback)
{
    LD_ASSERT(client); LD_ASSERT(key);

    LDNode *result;
    LDi_rdlock(&client->clientLock);

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeHash) {
        result = LDCloneHash(node->h);
    } else {
        result = LDCloneHash(fallback);
    }

    LDi_recordfeature(client, client->user, node, key, LDNodeHash,
        0.0, NULL, result, 0.0, NULL, fallback);

    LDi_rdunlock(&client->clientLock);

    return result;
}

void
LDClientTrack(LDClient *const client, const char *const name)
{
    LD_ASSERT(client); LD_ASSERT(name);
    LDi_rdlock(&client->clientLock);
    LDi_recordtrack(client, client->user, name, NULL);
    LDi_rdunlock(&client->clientLock);
}

void
LDClientTrackData(LDClient *const client, const char *const name, LDNode *const data)
{
    LD_ASSERT(client); LD_ASSERT(name); LD_ASSERT(data);
    LDi_rdlock(&client->clientLock);
    LDi_recordtrack(client, client->user, name, data);
    LDi_rdunlock(&client->clientLock);
}

void
LDClientFlush(LDClient *const client)
{
    LD_ASSERT(client); LDi_condsignal(&client->eventCond);
}

void
LDClientRegisterFeatureFlagListener(LDClient *const client, const char *const key, LDlistenerfn fn)
{
    LD_ASSERT(client != NULL); LD_ASSERT(key != NULL);

    struct listener *const list = LDAlloc(sizeof(*list)); LD_ASSERT(list);

    list->fn = fn;
    list->key = LDi_strdup(key);
    LD_ASSERT(list->key);

    LDi_wrlock(&client->clientLock);
    list->next = client->listeners;
    client->listeners = list;
    LDi_wrunlock(&client->clientLock);
}

bool
LDClientUnregisterFeatureFlagListener(LDClient *const client, const char *const key, LDlistenerfn fn)
{
    LD_ASSERT(client); LD_ASSERT(key);

    struct listener *list = NULL, *prev = NULL;

    LDi_wrlock(&client->clientLock);
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
    LDi_wrunlock(&client->clientLock);

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
LDi_updatestatus(struct LDClient_i *const client, const LDStatus status)
{
    if (client->status != status) {
        client->status = status;
        if (LDi_statuscallback) {
            LDi_wrunlock(&client->clientLock);
            LDi_statuscallback(status);
            LDi_wrlock(&client->clientLock);
        }
   }
   LDi_condsignal(&client->initCond);
};
