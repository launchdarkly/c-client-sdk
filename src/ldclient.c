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
    LDi_rwlock_init(&globalContext.sharedUserLock);

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
    LD_ASSERT(config->secondaryMobileKeys = LDNewObject());

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
    config->useReasons = false;
    config->proxyURI = NULL;
    config->verifyPeer = true;
    config->certFile = NULL;
    /* defaulting to true for now will be removed later */
    config->inlineUsersInEvents = true;

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
    LDJSONFree(config->privateAttributeNames);
    LDJSONFree(config->secondaryMobileKeys);
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
}

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
        LD_LOG(LD_LOG_CRITICAL, "no memory for the client");
        return NULL;
    }

    memset(client, 0, sizeof(*client));

    LDi_rwlock_init(&client->clientLock);

    LDi_rwlock_wrlock(&client->clientLock);

    client->shared = shared;
    client->offline = shared->sharedConfig->offline;
    client->background = false;
    client->status = LDStatusInitializing;

    client->shouldstopstreaming = false;
    client->streamhandle = 0;

    LD_ASSERT(LDSetString(&client->mobileKey, mobileKey));

    LD_ASSERT(client->eventProcessor =
        LDi_newEventProcessor(shared->sharedConfig));
    LD_ASSERT(LDi_storeInitialize(&client->store));

    LDi_mutex_init(&client->initCondMtx);
    LDi_cond_init(&client->initCond);

    LDi_mutex_init(&client->condMtx);

    LDi_cond_init(&client->eventCond);
    LDi_cond_init(&client->pollCond);
    LDi_cond_init(&client->streamCond);

    LDi_thread_create(&client->eventThread, LDi_bgeventsender, client);
    LDi_thread_create(&client->pollingThread, LDi_bgfeaturepoller, client);
    LDi_thread_create(&client->streamingThread, LDi_bgfeaturestreamer, client);

    LDi_rwlock_rdlock(&shared->sharedUserLock);
    char *const flags = NULL;
    // char *const flags = LDi_loaddata("features", shared->sharedUser->key);
    LDi_rwlock_rdunlock(&shared->sharedUserLock);

    if (flags) {
        // LDi_clientsetflags(client, false, flags, 1);
        LDFree(flags);
    }

    LDi_rwlock_rdlock(&shared->sharedUserLock);
    LD_ASSERT(LDi_identify(client->eventProcessor, shared->sharedUser));
    LDi_rwlock_rdunlock(&shared->sharedUserLock);
    LDi_rwlock_wrunlock(&client->clientLock);

    return client;
}

LDClient *
LDClientInit(LDConfig *const config, LDUser *const user,
    const unsigned int maxwaitmilli)
{
    struct LDJSON *secondaryKey, *tmp;

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

    for (secondaryKey = LDGetIter(config->secondaryMobileKeys); secondaryKey;
        secondaryKey = LDIterNext(secondaryKey))
    {
        const char *name;

        LD_ASSERT(name = LDIterKey(secondaryKey));

        LDClient *const secondaryClient = LDi_clientinitisolated(&globalContext,
            LDGetText(secondaryKey));

        LD_ASSERT(secondaryClient);

        HASH_ADD_KEYPTR(hh, globalContext.clientTable, name, strlen(name),
            secondaryClient);
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
        LDi_rwlock_wrlock(&clientIter->clientLock);
        clientIter->offline = true;
        LDi_rwlock_wrunlock(&clientIter->clientLock);
    }
}

void
LDClientSetOnline(LDClient *const client)
{
    LDClient *clientIter, *tmp;

    HASH_ITER(hh, globalContext.clientTable, clientIter, tmp) {
        LDi_rwlock_wrlock(&clientIter->clientLock);
        clientIter->offline = false;
        LDi_updatestatus(clientIter, LDStatusInitializing);
        LDi_rwlock_wrunlock(&clientIter->clientLock);
    }
}

bool
LDClientIsOffline(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_rwlock_rdlock(&client->clientLock);
    bool offline = client->offline;
    LDi_rwlock_rdunlock(&client->clientLock);
    return offline;
}

void
LDClientSetBackground(LDClient *const client, const bool background)
{
    LD_ASSERT(client);
    LDi_rwlock_wrlock(&client->clientLock);
    client->background = background;
    LDi_startstopstreaming(client, background);
    LDi_rwlock_wrunlock(&client->clientLock);
}

void
LDClientIdentify(LDClient *const client, LDUser *const user)
{
    LDClient *clientIter, *tmp;

    LD_ASSERT(client);
    LD_ASSERT(user);

    LDi_rwlock_wrlock(&globalContext.sharedUserLock);

    if (user != globalContext.sharedUser) {
        LDUserFree(globalContext.sharedUser);
    }

    globalContext.sharedUser = user;

    HASH_ITER(hh, globalContext.clientTable, clientIter, tmp) {
        LDi_rwlock_wrlock(&clientIter->clientLock);

        LDi_updatestatus(client, LDStatusInitializing);

        /*
        TODO load for specific user
        char *const flags = NULL;
        if (flags) {
            LDFree(flags);
        }
        */

        LDi_reinitializeconnection(clientIter);
        LD_ASSERT(LDi_identify(clientIter->eventProcessor, user));

        LDi_rwlock_wrunlock(&clientIter->clientLock);
    }

    LDi_rwlock_wrunlock(&globalContext.sharedUserLock);
}

void
clientCloseIsolated(LDClient *const client)
{
    LD_ASSERT(client);

    LDi_rwlock_wrlock(&client->clientLock);
    LDi_updatestatus(client, LDStatusShuttingdown);
    LDi_reinitializeconnection(client);
    LDi_rwlock_wrunlock(&client->clientLock);

    LDi_mutex_lock(&client->condMtx);
    LDi_cond_signal(&client->initCond);
    LDi_cond_signal(&client->eventCond);
    LDi_cond_signal(&client->pollCond);
    LDi_cond_signal(&client->streamCond);
    LDi_mutex_unlock(&client->condMtx);

    LDi_thread_join(&client->eventThread);
    LDi_thread_join(&client->pollingThread);
    LDi_thread_join(&client->streamingThread);

    LDi_freeEventProcessor(client->eventProcessor);
    LDi_storeDestroy(&client->store);

    LDi_rwlock_destroy(&client->clientLock);

    LDi_mutex_destroy(&client->initCondMtx);
    LDi_mutex_destroy(&client->condMtx);

    LDi_cond_destroy(&client->initCond);
    LDi_cond_destroy(&client->eventCond);
    LDi_cond_destroy(&client->pollCond);
    LDFree(client->mobileKey);

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

bool
LDClientIsInitialized(LDClient *const client)
{
    LD_ASSERT(client);
    LDi_rwlock_rdlock(&client->clientLock);
    bool isinit = client->status == LDStatusInitialized;
    LDi_rwlock_rdunlock(&client->clientLock);
    return isinit;
}

bool
LDClientAwaitInitialized(LDClient *const client, const unsigned int timeoutmilli)
{
    LD_ASSERT(client);
    LDi_mutex_lock(&client->initCondMtx);
    LDi_rwlock_rdlock(&client->clientLock);
    if (client->status == LDStatusInitialized) {
        LDi_rwlock_rdunlock(&client->clientLock);
        LDi_mutex_unlock(&client->initCondMtx);
        return true;
    }
    LDi_rwlock_rdunlock(&client->clientLock);

    LDi_cond_wait(&client->initCond, &client->initCondMtx, timeoutmilli);
    LDi_mutex_unlock(&client->initCondMtx);

    LDi_rwlock_rdlock(&client->clientLock);
    bool isinit = client->status == LDStatusInitialized;
    LDi_rwlock_rdunlock(&client->clientLock);
    return isinit;
}

void
LDSetClientStatusCallback(void (callback)(int))
{
    LDi_statuscallback = callback;
}

char *
LDClientSaveFlags(LDClient *const client)
{
    /* blank for now */
}

void
LDClientRestoreFlags(LDClient *const client, const char *const data)
{
    /* blank for now */
}

struct LDJSON *
LDAllFlags(LDClient *const client)
{
    struct LDJSON *result;
    struct LDStoreNode **flags;
    unsigned int flagCount, i;

    LD_ASSERT(client);

    LD_ASSERT(result = LDNewObject());

    LD_ASSERT(LDi_storeGetAll(&client->store, &flags, &flagCount));

    for (i = 0; i < flagCount; i++) {
        struct LDJSON *tmp;

        LD_ASSERT(tmp = LDJSONDuplicate(flags[i]->flag.value));

        LD_ASSERT(LDObjectSetKey(result, flags[i]->flag.key, tmp));

        LDi_rc_decrement(&flags[i]->rc);
    }

    LDFree(flags);

    return result;
}

/*
 * a block of functions to look up feature flags
 */

static void
fillDetails(const struct LDStoreNode *const node,
    LDVariationDetails *const details, const LDJSONType type)
{
    LD_ASSERT(details);

    if (node) {
        if (type == LDNull || LDJSONGetType(node->flag.value) == type ||
            LDJSONGetType(node->flag.value) == LDNull)
        {
            details->reason         = LDJSONDuplicate(node->flag.reason);
            details->variationIndex = node->flag.variation;
        } else {
            details->reason         = LDNewObject();
            details->variationIndex = -1;

            LDObjectSetKey(details->reason, "kind",
                LDNewText("ERROR"));
            LDObjectSetKey(details->reason, "errorKind",
                LDNewText("WRONG_TYPE"));
        }
    } else {
        details->reason         = LDNewObject();
        details->variationIndex = -1;

        LDObjectSetKey(details->reason, "kind",
            LDNewText("ERROR"));
        LDObjectSetKey(details->reason, "errorKind",
            LDNewText("FLAG_NOT_FOUND"));
    }
}

static void
LDi_castJSONToValue(
    void **const         destination,
    struct LDJSON *const source
) {
    LD_ASSERT(destination);
    LD_ASSERT(source);

    switch (LDJSONGetType(source)) {
        case LDNull:
            LD_ASSERT(false);
            break;

        case LDBool:
            **((bool **const)destination) = LDGetBool(source);
            break;

        case LDText:
            *((const char **const)destination) = LDGetText(source);
            break;

        case LDNumber:
            **((double **const)destination) = LDGetNumber(source);
            break;

        case LDObject:
            LD_ASSERT(false);
            break;

        case LDArray:
            LD_ASSERT(false);
            break;
    }
}

static bool
LDi_evalInternal(
    LDClient *const            client,
    const char *const          flagKey,
    const LDJSONType           variationKind,
    void *const                fallbackValue,
    void **const               resultValue,
    struct LDStoreNode **const selected
) {
    struct LDStoreNode *node;

    LD_ASSERT(client);
    LD_ASSERT(flagKey);
    LD_ASSERT(fallbackValue);
    LD_ASSERT(resultValue);

    node = LDi_storeGet(&client->store, flagKey);

    if (node && (variationKind == LDNull
        || LDJSONGetType(node->flag.value) == variationKind))
    {
        if (variationKind == LDNull) {
            *((struct LDJSON **const)resultValue) = node->flag.value;
        } else {
            LDi_castJSONToValue(resultValue, node->flag.value);
        }
    } else {
        *resultValue = fallbackValue;
    }

    LDi_rwlock_rdlock(&client->shared->sharedUserLock);

    LDi_processEvalEvent(
        client->eventProcessor,
        client->shared->sharedUser,
        flagKey,
        variationKind,
        node,
        *(const void **)resultValue,
        fallbackValue,
        (bool)selected
    );

    LDi_rwlock_rdunlock(&client->shared->sharedUserLock);

    if (selected) {
        *selected = node;
    } else if (node) {
        LDi_rc_decrement(&node->rc);
    }

    return true;
}

bool
LDBoolVariationDetail(LDClient *const client, const char *const key,
    bool fallback, LDVariationDetails *const details)
{
    bool value, *valueRef;
    struct LDStoreNode *selected;

    LD_ASSERT(client);
    LD_ASSERT(key);

    valueRef = &value;

    LDi_rwlock_rdlock(&client->clientLock);
    LDi_evalInternal(
        client, key, LDBool, &fallback, (void **)&valueRef, &selected
    );
    fillDetails(selected, details, LDBool);
    if (selected) {
        LDi_rc_decrement(&selected->rc);
    }
    LDi_rwlock_rdunlock(&client->clientLock);

    return *valueRef;
}

bool
LDBoolVariation(LDClient *const client, const char *const key,
    bool fallback)
{
    bool value, *valueRef;

    LD_ASSERT(client);
    LD_ASSERT(key);

    valueRef = &value;

    LDi_rwlock_rdlock(&client->clientLock);
    LDi_evalInternal(
        client, key, LDBool, &fallback, (void **)&valueRef, NULL
    );
    LDi_rwlock_rdunlock(&client->clientLock);

    return *valueRef;
}

int
LDIntVariationDetail(LDClient *const client, const char *const key,
    const int fallback, LDVariationDetails *const details)
{
    double value, *valueRef, fallbackCast;
    struct LDStoreNode *selected;

    LD_ASSERT(client);
    LD_ASSERT(key);

    valueRef     = &value;
    fallbackCast = fallback;

    LDi_rwlock_rdlock(&client->clientLock);
    LDi_evalInternal(
        client, key, LDNumber, &fallbackCast, (void **)&valueRef, &selected
    );
    fillDetails(selected, details, LDNumber);
    if (selected) {
        LDi_rc_decrement(&selected->rc);
    }
    LDi_rwlock_rdunlock(&client->clientLock);

    return *valueRef;
}

int
LDIntVariation(LDClient *const client, const char *const key,
    const int fallback)
{
    double value, *valueRef, fallbackCast;

    LD_ASSERT(client);
    LD_ASSERT(key);

    valueRef     = &value;
    fallbackCast = fallback;

    LDi_rwlock_rdlock(&client->clientLock);
    LDi_evalInternal(
        client, key, LDNumber, &fallbackCast, (void **)&valueRef, NULL
    );
    LDi_rwlock_rdunlock(&client->clientLock);

    return *valueRef;
}

double
LDDoubleVariationDetail(LDClient *const client, const char *const key,
    const double fallback, LDVariationDetails *const details)
{
    double value, *valueRef, fallbackCast;
    struct LDStoreNode *selected;

    LD_ASSERT(client);
    LD_ASSERT(key);

    valueRef     = &value;
    fallbackCast = fallback;

    LDi_rwlock_rdlock(&client->clientLock);
    LDi_evalInternal(
        client, key, LDNumber, &fallbackCast, (void **)&valueRef, &selected
    );
    fillDetails(selected, details, LDNumber);
    if (selected) {
        LDi_rc_decrement(&selected->rc);
    }
    LDi_rwlock_rdunlock(&client->clientLock);

    return *valueRef;
}

double
LDDoubleVariation(LDClient *const client, const char *const key,
    const double fallback)
{
    double value, *valueRef, fallbackCast;

    LD_ASSERT(client);
    LD_ASSERT(key);

    valueRef     = &value;
    fallbackCast = fallback;

    LDi_rwlock_rdlock(&client->clientLock);
    LDi_evalInternal(
        client, key, LDNumber, &fallbackCast, (void **)&valueRef, NULL
    );
    LDi_rwlock_rdunlock(&client->clientLock);

    return *valueRef;
}

char *
LDStringVariationDetail(LDClient *const client, const char *const key,
    const char *const fallback, char *const buffer, const size_t bufferSize,
    LDVariationDetails *const details)
{
    size_t resultLength;
    char *value;
    struct LDStoreNode *selected;

    LD_ASSERT(client);
    LD_ASSERT(key);
    LD_ASSERT(!(!buffer && bufferSize));

    LDi_rwlock_rdlock(&client->clientLock);
    LDi_evalInternal(
        client, key, LDText, (void *)fallback, (void **)&value, &selected
    );
    fillDetails(selected, details, LDText);
    if (selected) {
        LDi_rc_decrement(&selected->rc);
    }
    LDi_rwlock_rdunlock(&client->clientLock);

    resultLength = strlen(value);
    memcpy(buffer, value, resultLength);
    buffer[resultLength] = '\0';

    return buffer;
}

char *
LDStringVariation(LDClient *const client, const char *const key,
    const char *const fallback, char *const buffer, const size_t bufferSize)
{
    size_t resultLength;
    char *value;

    LD_ASSERT(client);
    LD_ASSERT(key);
    LD_ASSERT(!(!buffer && bufferSize));

    LDi_rwlock_rdlock(&client->clientLock);
    LDi_evalInternal(
        client, key, LDText, (void *)fallback, (void **)&value, NULL
    );
    LDi_rwlock_rdunlock(&client->clientLock);

    resultLength = strlen(value);
    memcpy(buffer, value, resultLength);
    buffer[resultLength] = '\0';

    return buffer;
}

char *
LDStringVariationAllocDetail(LDClient *const client, const char *const key,
    const char* fallback, LDVariationDetails *const details)
{
    char *value;
    struct LDStoreNode *selected;

    LD_ASSERT(client);
    LD_ASSERT(key);
    LD_ASSERT(fallback);

    LDi_rwlock_rdlock(&client->clientLock);
    LDi_evalInternal(
        client, key, LDText, (void *)fallback, (void **)&value, &selected
    );
    fillDetails(selected, details, LDText);
    if (selected) {
        LDi_rc_decrement(&selected->rc);
    }
    LDi_rwlock_rdunlock(&client->clientLock);

    return LDStrDup(value);
}

char *
LDStringVariationAlloc(LDClient *const client, const char *const key,
    const char* fallback)
{
    char *value;

    LD_ASSERT(client);
    LD_ASSERT(key);
    LD_ASSERT(fallback);

    LDi_rwlock_rdlock(&client->clientLock);
    LDi_evalInternal(
        client, key, LDText, (void *)fallback, (void **)&value, NULL
    );
    LDi_rwlock_rdunlock(&client->clientLock);

    return LDStrDup(value);
}

struct LDJSON *
LDJSONVariationDetail(LDClient *const client, const char *const key,
    struct LDJSON *const fallback, LDVariationDetails *const details)
{
    struct LDJSON *value;
    struct LDStoreNode *selected;

    LD_ASSERT(client);
    LD_ASSERT(key);
    LD_ASSERT(fallback);

    LDi_rwlock_rdlock(&client->clientLock);
    LDi_evalInternal(
        client, key, LDNull, (void *)fallback, (void **)&value, &selected
    );
    fillDetails(selected, details, LDNull);
    if (selected) {
        LDi_rc_decrement(&selected->rc);
    }
    LDi_rwlock_rdunlock(&client->clientLock);

    return LDJSONDuplicate(value);
}

struct LDJSON *
LDJSONVariation(LDClient *const client, const char *const key,
    struct LDJSON *const fallback)
{
    struct LDJSON *value;

    LD_ASSERT(client);
    LD_ASSERT(key);
    LD_ASSERT(fallback);

    LDi_rwlock_rdlock(&client->clientLock);
    LDi_evalInternal(
        client, key, LDNull, (void *)fallback, (void **)&value, NULL
    );
    LDi_rwlock_rdunlock(&client->clientLock);

    return LDJSONDuplicate(value);
}

void
LDClientTrack(LDClient *const client, const char *const name)
{
    LD_ASSERT(client);
    LD_ASSERT(name);

    LDi_rwlock_rdlock(&client->shared->sharedUserLock);
    LDi_track(client->eventProcessor, client->shared->sharedUser, name,
        NULL, 0, false);
    LDi_rwlock_rdunlock(&client->shared->sharedUserLock);
}

void
LDClientTrackData(LDClient *const client, const char *const name,
    struct LDJSON *const data)
{
    LD_ASSERT(client);
    LD_ASSERT(name);

    LDi_rwlock_rdlock(&client->shared->sharedUserLock);
    LDi_track(client->eventProcessor, client->shared->sharedUser, name,
        data, 0, false);
    LDi_rwlock_rdunlock(&client->shared->sharedUserLock);
}

void
LDClientTrackMetric(LDClient *const client, const char *const name,
    struct LDJSON *const data, const double metric)
{
    LD_ASSERT(client);
    LD_ASSERT(name);

    LDi_rwlock_rdlock(&client->shared->sharedUserLock);
    LDi_track(client->eventProcessor, client->shared->sharedUser, name,
        data, metric, true);
    LDi_rwlock_rdunlock(&client->shared->sharedUserLock);
}

void
LDClientFlush(LDClient *const client)
{
    LDClient *clientIter, *tmp;

    HASH_ITER(hh, globalContext.clientTable, clientIter, tmp) {
        LDi_cond_signal(&clientIter->eventCond);
    }
}

void
LDClientRegisterFeatureFlagListener(LDClient *const client, const char *const key, LDlistenerfn fn)
{
    LD_ASSERT(client != NULL); LD_ASSERT(key != NULL);

    struct listener *const list = LDAlloc(sizeof(*list)); LD_ASSERT(list);

    list->fn = fn;
    list->key = LDStrDup(key);
    LD_ASSERT(list->key);

    LDi_rwlock_wrlock(&client->clientLock);
    list->next = client->listeners;
    client->listeners = list;
    LDi_rwlock_wrunlock(&client->clientLock);
}

bool
LDClientUnregisterFeatureFlagListener(LDClient *const client, const char *const key, LDlistenerfn fn)
{
    LD_ASSERT(client); LD_ASSERT(key);

    struct listener *list = NULL, *prev = NULL;

    LDi_rwlock_wrlock(&client->clientLock);
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
    LDi_rwlock_wrunlock(&client->clientLock);

    return list != NULL;
}

void
LDConfigSetPrivateAttributes(LDConfig *const config,
    struct LDJSON *attributes)
{
    LD_ASSERT(config);

    if (attributes) {
        LD_ASSERT(LDJSONGetType(attributes) == LDArray);

        LDJSONFree(config->privateAttributeNames);
    }

    config->privateAttributeNames = attributes;
}

bool
LDConfigAddSecondaryMobileKey(LDConfig *const config, const char *const name,
    const char *const key)
{
    struct LDJSON *tmp;

    LD_ASSERT(config);
    LD_ASSERT(name);
    LD_ASSERT(key);

    if (strcmp(name, LDPrimaryEnvironmentName) == 0) {
        LD_LOG(LD_LOG_ERROR,
            "Attempted use the primary environment name as secondary");

        return false;
    }

    if (strcmp(key, config->mobileKey) == 0) {
        LD_LOG(LD_LOG_ERROR,
            "Attempted to add primary key as secondary key");

        return false;
    }

    tmp = LDObjectLookup(config->secondaryMobileKeys, name);

    if (tmp && strcmp(LDGetText(tmp), name) == 0) {
        LD_LOG(LD_LOG_ERROR, "Attempted to add secondary key twice");

        return false;
    }

    if (!(tmp = LDNewText(key))) {
        LD_LOG(LD_LOG_ERROR,
            "LDConfigAddSecondaryMobileKey failed to duplicate key");

        return false;
    }

    if (!LDObjectSetKey(config->secondaryMobileKeys, name, tmp)) {
        LDJSONFree(tmp);

        LD_LOG(LD_LOG_ERROR,
            "LDConfigAddSecondaryMobileKey failed to add environment");

        return false;
    }

    return true;
}

void
LDi_updatestatus(struct LDClient_i *const client, const LDStatus status)
{
    if (client->status != status) {
        client->status = status;
        if (LDi_statuscallback) {
            LDi_rwlock_wrunlock(&client->clientLock);
            LDi_statuscallback(status);
            LDi_rwlock_wrlock(&client->clientLock);
        }
   }
   LDi_cond_signal(&client->initCond);
}

void
LDFreeDetailContents(LDVariationDetails details)
{
    LDJSONFree(details.reason);
}
