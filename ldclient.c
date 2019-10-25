#include <stdlib.h>
#include <stdio.h>
#ifndef _WINDOWS
#include <unistd.h>
#endif
#include <math.h>

#include "curl/curl.h"

#include "uthash.h"
#include "ldapi.h"
#include "ldinternal.h"

static struct LDGlobal_i globalContext = {
    NULL, NULL, NULL, NULL, LD_RWLOCK_INIT
};

ld_once_t LDi_earlyonce = LD_ONCE_INIT;

void (*LDi_statuscallback)(int);

void
LDi_earlyinit(void)
{
    LDi_mtxinit(&LDi_allocmtx);

    LDi_rwlockinit(&globalContext.sharedUserLock);

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
    config->secondaryMobileKeys = NULL;
    config->streaming = true;
    config->useReport = false;
    config->useReasons = false;
    config->proxyURI = NULL;
    config->verifyPeer = true;
    config->certFile = NULL;

    return config;
}

void
LDConfigSetAllAttributesPrivate(LDConfig *const config,
    const bool private)
{
    LD_ASSERT(config); config->allAttributesPrivate = private;
}

void
LDConfigSetBackgroundPollingIntervalMillis(LDConfig *const config,
    const int millis)
{
    LD_ASSERT(config); config->backgroundPollingIntervalMillis = millis;
}

void
LDConfigSetAppURI(LDConfig *const config, const char *const uri)
{
    LD_ASSERT(config); LD_ASSERT(uri); LD_ASSERT(LDSetString(&config->appURI, uri));
}

void
LDConfigSetConnectionTimeoutMillies(LDConfig *const config,
    const int millis)
{
    LD_ASSERT(config); config->connectionTimeoutMillis = millis;
}

void
LDConfigSetDisableBackgroundUpdating(LDConfig *const config,
    const bool disable)
{
    LD_ASSERT(config); config->disableBackgroundUpdating = disable;
}

void
LDConfigSetEventsCapacity(LDConfig *const config, const int capacity)
{
    LD_ASSERT(config); config->eventsCapacity = capacity;
}

void
LDConfigSetEventsFlushIntervalMillis(LDConfig *const config,
    const int millis)
{
    LD_ASSERT(config); config->eventsFlushIntervalMillis = millis;
}

void
LDConfigSetEventsURI(LDConfig *const config, const char *const uri)
{
    LD_ASSERT(config); LD_ASSERT(uri); LD_ASSERT(LDSetString(&config->eventsURI, uri));
}

void
LDConfigSetMobileKey(LDConfig *const config, const char *const key)
{
    LD_ASSERT(config); LD_ASSERT(key); LD_ASSERT(LDSetString(&config->mobileKey, key));
}

void
LDConfigSetOffline(LDConfig *const config, const bool offline)
{
    LD_ASSERT(config); config->offline = offline;
}

void
LDConfigSetStreaming(LDConfig *const config, const bool streaming)
{
    LD_ASSERT(config); config->streaming = streaming;
}

void
LDConfigSetPollingIntervalMillis(LDConfig *const config, const int millis)
{
    LD_ASSERT(config); config->pollingIntervalMillis = millis;
}

void
LDConfigSetStreamURI(LDConfig *const config, const char *const uri)
{
    LD_ASSERT(config); LD_ASSERT(uri); LD_ASSERT(LDSetString(&config->streamURI, uri));
}

void
LDConfigSetUseReport(LDConfig *const config, const bool report)
{
    LD_ASSERT(config); config->useReport = report;
}

void
LDConfigSetUseEvaluationReasons(LDConfig *const config, const bool reasons)
{
    LD_ASSERT(config); config->useReasons = reasons;
}

void
LDConfigSetProxyURI(LDConfig *const config, const char *const uri)
{
    LD_ASSERT(config); LD_ASSERT(uri); LD_ASSERT(LDSetString(&config->proxyURI, uri));
}

void
LDConfigSetVerifyPeer(LDConfig *const config, const bool enabled)
{
    LD_ASSERT(config); config->verifyPeer = enabled;
}

void LDConfigSetSSLCertificateAuthority(LDConfig *const config, const char *const certFile)
{
    LD_ASSERT(config); LD_ASSERT(LDSetString(&config->certFile, certFile));
}

void
LDConfigFree(LDConfig *const config)
{
    if (!config) { return; }
    LDFree(config->appURI);
    LDFree(config->eventsURI);
    LDFree(config->mobileKey);
    LDFree(config->streamURI);
    LDFree(config->proxyURI);
    LDFree(config->certFile);
    LDi_freehash(config->privateAttributeNames);
    LDi_freehash(config->secondaryMobileKeys);
    LDFree(config);
}

static void
checkconfig(LDConfig *config)
{
    if (config->pollingIntervalMillis < 300000) {
        config->pollingIntervalMillis = 300000;
    }
}

LDClient *
LDClientGet()
{
    return globalContext.primaryClient;
};

struct LDClient_i *
LDClientGetForMobileKey(const char *keyName)
{
    struct LDClient_i *lookup;

    LD_ASSERT(keyName);

    HASH_FIND_STR(globalContext.clientTable, keyName, lookup);

    return lookup;
}

LDClient *
LDi_clientinitisolated(struct LDGlobal_i *const shared,
    const char *const mobileKey)
{
    LD_ASSERT(shared); LD_ASSERT(mobileKey);

    LDi_once(&LDi_earlyonce, LDi_earlyinit);

    LDClient *const client = LDAlloc(sizeof(*client));

    if (!client) {
        LDi_log(LD_LOG_CRITICAL, "no memory for the client");
        return NULL;
    }

    memset(client, 0, sizeof(*client));

    LDi_rwlockinit(&client->clientLock);

    LDi_wrlock(&client->clientLock);

    client->shared = shared;
    client->offline = shared->sharedConfig->offline;
    client->background = false;
    client->status = LDStatusInitializing;
    client->allFlags = NULL;

    client->shouldstopstreaming = false;
    client->databuffer = NULL;
    client->streamhandle = 0;

    LD_ASSERT(LDSetString(&client->mobileKey, mobileKey));

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

    LDi_rdlock(&shared->sharedUserLock);
    char *const flags = LDi_loaddata("features", shared->sharedUser->key);
    LDi_rdunlock(&shared->sharedUserLock);

    if (flags) {
        LDi_clientsetflags(client, false, flags, 1);
        LDFree(flags);
    }

    LDi_rdlock(&shared->sharedUserLock);
    LDi_recordidentify(client, shared->sharedUser);
    LDi_rdunlock(&shared->sharedUserLock);
    LDi_wrunlock(&client->clientLock);

    return client;
}

LDClient *
LDClientInit(LDConfig *const config, LDUser *const user,
    const unsigned int maxwaitmilli)
{
    const LDNode *secondaryKey, *tmp;

    LD_ASSERT(config); LD_ASSERT(user); LD_ASSERT(!globalContext.primaryClient);

    checkconfig(config);

    globalContext.sharedUser   = user;
    globalContext.sharedConfig = config;

    globalContext.primaryClient = LDi_clientinitisolated(&globalContext,
        config->mobileKey);

    LD_ASSERT(globalContext.primaryClient);

    HASH_ADD_KEYPTR(hh, globalContext.clientTable,
        LDPrimaryEnvironmentName, strlen(LDPrimaryEnvironmentName),
        globalContext.primaryClient);

    HASH_ITER(hh, config->secondaryMobileKeys, secondaryKey, tmp) {
        LDClient *const secondaryClient = LDi_clientinitisolated(&globalContext,
            secondaryKey->s);

        LD_ASSERT(secondaryClient);

        HASH_ADD_KEYPTR(hh, globalContext.clientTable, secondaryKey->key,
            strlen(secondaryKey->key), secondaryClient);
    }

    if (maxwaitmilli) {
        struct LDClient_i *clientIter, *clientTmp;

        const unsigned long long future = 1000 * (unsigned long long)time(NULL) + maxwaitmilli;

        HASH_ITER(hh, globalContext.clientTable, clientIter, clientTmp) {
            const unsigned long long now = 1000 * (unsigned long long)time(NULL);

            if (now < future) {
                LDClientAwaitInitialized(clientIter, future - now);
            } else {
                break;
            }
        }
    }

    return globalContext.primaryClient;
}

void
LDClientSetOffline(LDClient *const client)
{
    LDClient *clientIter, *tmp;

    HASH_ITER(hh, globalContext.clientTable, clientIter, tmp) {
        LDi_wrlock(&clientIter->clientLock);
        clientIter->offline = true;
        LDi_wrunlock(&clientIter->clientLock);
    }
}

void
LDClientSetOnline(LDClient *const client)
{
    LDClient *clientIter, *tmp;

    HASH_ITER(hh, globalContext.clientTable, clientIter, tmp) {
        LDi_wrlock(&clientIter->clientLock);
        clientIter->offline = false;
        LDi_updatestatus(clientIter, LDStatusInitializing);
        LDi_wrunlock(&clientIter->clientLock);
    }
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
    LDClient *clientIter, *tmp;

    LD_ASSERT(client); LD_ASSERT(user);

    LDi_wrlock(&globalContext.sharedUserLock);

    if (user != globalContext.sharedUser) {
        LDUserFree(globalContext.sharedUser);
    }

    globalContext.sharedUser = user;

    HASH_ITER(hh, globalContext.clientTable, clientIter, tmp) {
        LDi_wrlock(&clientIter->clientLock);

        LDi_freehash(clientIter->allFlags);
        clientIter->allFlags = NULL;

        if (clientIter->status == LDStatusInitialized) {
            LDi_updatestatus(client, LDStatusInitializing);
        }

        char *const flags = LDi_loaddata("features", user->key);
        if (flags) {
            LDi_clientsetflags(clientIter, false, flags, 1);
            LDFree(flags);
        }

        LDi_reinitializeconnection(clientIter);
        LDi_recordidentify(clientIter, user);

        LDi_wrunlock(&clientIter->clientLock);
    }

    LDi_wrunlock(&globalContext.sharedUserLock);
}

void
clientCloseIsolated(LDClient *const client)
{
    LD_ASSERT(client);

    LDi_wrlock(&client->clientLock);
    LDi_updatestatus(client, LDStatusShuttingdown);
    LDi_reinitializeconnection(client);
    LDi_wrunlock(&client->clientLock);

    LDi_mtxenter(&client->condMtx);
    LDi_condsignal(&client->initCond);
    LDi_condsignal(&client->eventCond);
    LDi_condsignal(&client->pollCond);
    LDi_condsignal(&client->streamCond);
    LDi_mtxleave(&client->condMtx);

    LDi_jointhread(client->eventThread);
    LDi_jointhread(client->pollingThread);
    LDi_jointhread(client->streamingThread);

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
    LDFree(client->mobileKey);
    LDFree(client->databuffer);

    for (struct listener *item = client->listeners; item;) {
        /* must record next to make delete safe */
        struct listener *const next = item->next;
        LDFree(item->key); LDFree(item);
        item = next;
    }

    LDFree(client);
}

void
LDClientClose(LDClient *const client)
{
    LDClient *clientIter, *tmp;

    HASH_ITER(hh, globalContext.clientTable, clientIter, tmp) {
        HASH_DEL(globalContext.clientTable, clientIter);
        clientCloseIsolated(clientIter);
    }

    LDUserFree(globalContext.sharedUser);
    LDConfigFree(globalContext.sharedConfig);

    globalContext.sharedConfig  = NULL;
    globalContext.primaryClient = NULL;
    globalContext.clientTable   = NULL;
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
        LDi_rdlock(&client->shared->sharedUserLock);
        LDi_savedata("features", client->shared->sharedUser->key, data);
        LDi_rdunlock(&client->shared->sharedUserLock);
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

    if (!cJSON_IsObject(payload)) {
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
    LDi_rdlock(&client->shared->sharedUserLock);
    char *const serialized = LDi_hashtostring(client->allFlags, true);
    LDi_savedata("features", client->shared->sharedUser->key, serialized);
    LDi_rdunlock(&client->shared->sharedUserLock);
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

static void
fillDetails(const LDNode *const node, LDVariationDetails *const details, const LDNodeType type)
{
    if (node) {
        if (node->type == type || node->type == LDNodeNone) {
            details->reason = LDCloneHash(node->reason);
            details->variationIndex = node->variation;
        } else {
            details->reason = LDNodeCreateHash();
            details->variationIndex = -1;
            LDNodeAddString(&details->reason, "kind", "ERROR");
            LDNodeAddString(&details->reason, "errorKind", "WRONG_TYPE");
        }
    } else {
        details->reason = LDNodeCreateHash();
        details->variationIndex = -1;
        LDNodeAddString(&details->reason, "kind", "ERROR");
        LDNodeAddString(&details->reason, "errorKind", "FLAG_NOT_FOUND");
    }
}

static bool
LDi_BoolNode(LDClient *const client, const char *const key, const bool fallback, LDNode **const selected)
{
    bool result;

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeBool) {
        result = node->b;
    } else {
        result = fallback;
    }

    LDi_rdlock(&client->shared->sharedUserLock);
    LDi_recordfeature(client, client->shared->sharedUser, node, key, LDNodeBool,
        (double)result, NULL, NULL, (double)fallback, NULL, NULL, (bool)selected);
    LDi_rdunlock(&client->shared->sharedUserLock);

    if (selected) { *selected = node; }

    return result;
}

bool
LDBoolVariationDetail(LDClient *const client, const char *const key,
    const bool fallback, LDVariationDetails *const details)
{
    LD_ASSERT(client); LD_ASSERT(key); LD_ASSERT(details);

    LDi_rdlock(&client->clientLock);
    LDNode *selected = NULL;
    const bool value = LDi_BoolNode(client, key, fallback, &selected);
    fillDetails(selected, details, LDNodeBool);
    LDi_rdunlock(&client->clientLock);

    return value;
}

bool
LDBoolVariation(LDClient *const client, const char *const key, const bool fallback)
{
    LD_ASSERT(client); LD_ASSERT(key);

    LDi_rdlock(&client->clientLock);
    const bool value = LDi_BoolNode(client, key, fallback, NULL);
    LDi_rdunlock(&client->clientLock);

    return value;
}

static int
LDi_IntNode(LDClient *const client, const char *const key,
    const int fallback, LDNode **const selected)
{
    int result;

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeNumber) {
        result = (int)node->n;
    } else {
        result = fallback;
    }

    LDi_rdlock(&client->shared->sharedUserLock);
    LDi_recordfeature(client, client->shared->sharedUser, node, key,
        LDNodeNumber, (double)result, NULL, NULL, (double)fallback, NULL, NULL,
        (bool)selected);
    LDi_rdunlock(&client->shared->sharedUserLock);

    if (selected) { *selected = node; }

    return result;
}

int
LDIntVariationDetail(LDClient *const client, const char *const key,
    const int fallback, LDVariationDetails *const details)
{
    LD_ASSERT(client); LD_ASSERT(key); LD_ASSERT(details);

    LDi_rdlock(&client->clientLock);
    LDNode *selected = NULL;
    const int value = LDi_IntNode(client, key, fallback, &selected);
    fillDetails(selected, details, LDNodeNumber);
    LDi_rdunlock(&client->clientLock);

    return value;
}

int
LDIntVariation(LDClient *const client, const char *const key, const int fallback)
{
    LD_ASSERT(client); LD_ASSERT(key);

    LDi_rdlock(&client->clientLock);
    const int value = LDi_IntNode(client, key, fallback, NULL);
    LDi_rdunlock(&client->clientLock);

    return value;
}

double
LDi_DoubleNode(LDClient *const client, const char *const key,
    const double fallback, LDNode **const selected)
{
    double result;

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeNumber) {
        result = node->n;
    } else {
        result = fallback;
    }

    LDi_rdlock(&client->shared->sharedUserLock);
    LDi_recordfeature(client, client->shared->sharedUser, node, key,
        LDNodeNumber, result, NULL, NULL, fallback, NULL, NULL, (bool)selected);
    LDi_rdunlock(&client->shared->sharedUserLock);

    if (selected) { *selected = node; }

    return result;
}

double
LDDoubleVariationDetail(LDClient *const client, const char *const key,
    const double fallback, LDVariationDetails *const details)
{
    LD_ASSERT(client); LD_ASSERT(key); LD_ASSERT(details);

    LDi_rdlock(&client->clientLock);
    LDNode *selected = NULL;
    const double value = LDi_DoubleNode(client, key, fallback, &selected);
    fillDetails(selected, details, LDNodeNumber);
    LDi_rdunlock(&client->clientLock);

    return value;
}

double
LDDoubleVariation(LDClient *const client, const char *const key, const double fallback)
{
    LD_ASSERT(client); LD_ASSERT(key);

    LDi_rdlock(&client->clientLock);
    const double value = LDi_DoubleNode(client, key, fallback, NULL);
    LDi_rdunlock(&client->clientLock);

    return value;
}

static char *
LDi_StringNode(LDClient *const client, const char *const key,
    const char *const fallback, char *const buffer, const size_t space, LDNode **const selected)
{
    const char *result;

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

    LDi_rdlock(&client->shared->sharedUserLock);
    LDi_recordfeature(client, client->shared->sharedUser, node, key,
        LDNodeString, 0.0, result, NULL, 0.0, fallback, NULL, (bool)selected);
    LDi_rdunlock(&client->shared->sharedUserLock);

    if (selected) { *selected = node; }

    return buffer;
}

char *
LDStringVariationDetail(LDClient *const client, const char *const key,
    const char *const fallback, char *const buffer, const size_t space, LDVariationDetails *const details)
{
    LD_ASSERT(client); LD_ASSERT(key); LD_ASSERT(!(!buffer && space));  LD_ASSERT(details);

    LDi_rdlock(&client->clientLock);
    LDNode *selected = NULL;
    char* const value = LDi_StringNode(client, key, fallback, buffer, space, &selected);
    fillDetails(selected, details, LDNodeString);
    LDi_rdunlock(&client->clientLock);

    return value;
}

char *
LDStringVariation(LDClient *const client, const char *const key,
    const char *const fallback, char *const buffer, const size_t space)
{
    LD_ASSERT(client); LD_ASSERT(key); LD_ASSERT(!(!buffer && space));

    LDi_rdlock(&client->clientLock);
    char* const value = LDi_StringNode(client, key, fallback, buffer, space, NULL);
    LDi_rdunlock(&client->clientLock);

    return value;
}

static char *
LDi_StringAllocNode(LDClient *const client, const char *const key,
    const char *const fallback, LDNode **const selected)
{
    const char *value;

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeString) {
        value = node->s;
    } else {
        value = fallback;
    }

    char *const result = LDi_strdup(value);

    LDi_rdlock(&client->shared->sharedUserLock);
    LDi_recordfeature(client, client->shared->sharedUser, node, key,
        LDNodeString, 0.0, result, NULL, 0.0, fallback, NULL, (bool)selected);
    LDi_rdunlock(&client->shared->sharedUserLock);

    if (selected) { *selected = node; }

    return result;
}

char *
LDStringVariationAllocDetail(LDClient *const client, const char *const key,
    const char* fallback, LDVariationDetails *const details)
{
    LD_ASSERT(client); LD_ASSERT(key); LD_ASSERT(details);

    LDi_rdlock(&client->clientLock);
    LDNode *selected = NULL;
    char *const value = LDi_StringAllocNode(client, key, fallback, &selected);
    fillDetails(selected, details, LDNodeString);
    LDi_rdunlock(&client->clientLock);

    return value;
}

char *
LDStringVariationAlloc(LDClient *const client, const char *const key, const char* fallback)
{
    LD_ASSERT(client); LD_ASSERT(key);

    LDi_rdlock(&client->clientLock);
    char *const value = LDi_StringAllocNode(client, key, fallback, NULL);
    LDi_rdunlock(&client->clientLock);

    return value;
}

static LDNode *
LDi_JSONNode(LDClient *const client, const char *const key,
    const LDNode *const fallback, LDNode **const selected)
{
    LDNode *result;

    LDNode *const node = LDNodeLookup(client->allFlags, key);

    if (node && node->type == LDNodeHash) {
        result = LDCloneHash(node->h);
    } else {
        result = LDCloneHash(fallback);
    }

    LDi_rdlock(&client->shared->sharedUserLock);
    LDi_recordfeature(client, client->shared->sharedUser, node, key,
        LDNodeHash, 0.0, NULL, result, 0.0, NULL, fallback, (bool)selected);
    LDi_rdunlock(&client->shared->sharedUserLock);

    if (selected) { *selected = node; }

    return result;
}

LDNode *
LDJSONVariationDetail(LDClient *const client, const char *const key,
    const LDNode* const fallback, LDVariationDetails *const details)
{
    LD_ASSERT(client); LD_ASSERT(key); LD_ASSERT(details);

    LDi_rdlock(&client->clientLock);
    LDNode *selected = NULL;
    LDNode *const value = LDi_JSONNode(client, key, fallback, &selected);
    fillDetails(selected, details, LDNodeHash);
    LDi_rdunlock(&client->clientLock);

    return value;
}

LDNode *
LDJSONVariation(LDClient *const client, const char *const key, const LDNode *const fallback)
{
    LD_ASSERT(client); LD_ASSERT(key);

    LDi_rdlock(&client->clientLock);
    LDNode *const value = LDi_JSONNode(client, key, fallback, NULL);
    LDi_rdunlock(&client->clientLock);

    return value;
}

void
LDClientTrack(LDClient *const client, const char *const name)
{
    LD_ASSERT(client); LD_ASSERT(name);
    LDi_rdlock(&client->shared->sharedUserLock);
    LDi_recordtrack(client, client->shared->sharedUser, name, NULL);
    LDi_rdunlock(&client->shared->sharedUserLock);
}

void
LDClientTrackData(LDClient *const client, const char *const name, LDNode *const data)
{
    LD_ASSERT(client); LD_ASSERT(name);
    LDi_rdlock(&client->shared->sharedUserLock);
    LDi_recordtrack(client, client->shared->sharedUser, name, data);
    LDi_rdunlock(&client->shared->sharedUserLock);
}

void
LDClientTrackMetric(LDClient *const client, const char *const name,
    LDNode *const data, const double metric)
{
    LD_ASSERT(client); LD_ASSERT(name);
    LDi_rdlock(&client->shared->sharedUserLock);
    LDi_recordtrackmetric(client, client->shared->sharedUser, name, data,
        metric);
    LDi_rdunlock(&client->shared->sharedUserLock);
}

void
LDClientFlush(LDClient *const client)
{
    LDClient *clientIter, *tmp;

    HASH_ITER(hh, globalContext.clientTable, clientIter, tmp) {
        LDi_condsignal(&clientIter->eventCond);
    }
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
    const LDNode *lookup;

    LD_ASSERT(config); LD_ASSERT(key);

    HASH_FIND_STR(config->privateAttributeNames, key, lookup);

    if (lookup) {
        LDi_log(LD_LOG_WARNING, "Attempted to add duplicate private Attribute");

        return;
    }

    LDNode *const node = LDAlloc(sizeof(*node)); LD_ASSERT(node);
    memset(node, 0, sizeof(*node));

    node->key = LDi_strdup(key);
    LD_ASSERT(node->key);

    node->type = LDNodeNone;

    HASH_ADD_KEYPTR(hh, config->privateAttributeNames, node->key, strlen(node->key), node);
}

bool
LDConfigAddSecondaryMobileKey(LDConfig *const config, const char *const name,
    const char *const key)
{
    const LDNode *iter, *tmp;

    LD_ASSERT(config); LD_ASSERT(name); LD_ASSERT(key);

    if (strcmp(name, LDPrimaryEnvironmentName) == 0) {
        LDi_log(LD_LOG_ERROR, "Attempted use the primary environment name as secondary");

        return false;
    }

    if (strcmp(key, config->mobileKey) == 0) {
        LDi_log(LD_LOG_ERROR, "Attempted to add primary key as secondary key");

        return false;
    }

    HASH_ITER(hh, config->secondaryMobileKeys, iter, tmp) {
        if (strcmp(iter->key, name) == 0 || strcmp(iter->s, key) == 0) {
            LDi_log(LD_LOG_ERROR, "Attempted to add secondary key twice");

            return false;
        }
    }

    LDNodeAddString(&config->secondaryMobileKeys, name, key);

    return true;
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

void LDFreeDetailContents(LDVariationDetails details)
{
    LDi_freehash(details.reason);
}
