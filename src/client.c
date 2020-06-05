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

struct LDClient *
LDClientGet()
{
    return globalContext.primaryClient;
}

struct LDClient *
LDClientGetForMobileKey(const char *keyName)
{
    struct LDClient *lookup;

    LD_ASSERT(keyName);

    HASH_FIND_STR(globalContext.clientTable, keyName, lookup);

    return lookup;
}

struct LDClient *
LDi_clientinitisolated(struct LDGlobal_i *const shared,
    const char *const mobileKey)
{
    LD_ASSERT(shared);
    LD_ASSERT(mobileKey);

    LDi_once(&LDi_earlyonce, LDi_earlyinit);

    struct LDClient *const client = LDAlloc(sizeof(*client));

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

struct LDClient *
LDClientInit(struct LDConfig *const config, struct LDUser *const user,
    const unsigned int maxwaitmilli)
{
    struct LDJSON *secondaryKey, *tmp;

    LD_ASSERT(config);
    LD_ASSERT(user);
    LD_ASSERT(!globalContext.primaryClient);

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

        struct LDClient *const secondaryClient = LDi_clientinitisolated(&globalContext,
            LDGetText(secondaryKey));

        LD_ASSERT(secondaryClient);

        HASH_ADD_KEYPTR(hh, globalContext.clientTable, name, strlen(name),
            secondaryClient);
    }

    if (maxwaitmilli) {
        struct LDClient *clientIter, *clientTmp;

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
LDClientSetOffline(struct LDClient *const client)
{
    struct LDClient *clientIter, *tmp;

    HASH_ITER(hh, globalContext.clientTable, clientIter, tmp) {
        LDi_rwlock_wrlock(&clientIter->clientLock);
        clientIter->offline = true;
        LDi_rwlock_wrunlock(&clientIter->clientLock);
    }
}

void
LDClientSetOnline(struct LDClient *const client)
{
    struct LDClient *clientIter, *tmp;

    HASH_ITER(hh, globalContext.clientTable, clientIter, tmp) {
        LDi_rwlock_wrlock(&clientIter->clientLock);
        clientIter->offline = false;
        LDi_updatestatus(clientIter, LDStatusInitializing);
        LDi_rwlock_wrunlock(&clientIter->clientLock);
    }
}

LDBoolean
LDClientIsOffline(struct LDClient *const client)
{
    LD_ASSERT(client);
    LDi_rwlock_rdlock(&client->clientLock);
    bool offline = client->offline;
    LDi_rwlock_rdunlock(&client->clientLock);
    return offline;
}

void
LDClientSetBackground(struct LDClient *const client, const LDBoolean background)
{
    LD_ASSERT(client);
    LDi_rwlock_wrlock(&client->clientLock);
    client->background = background;
    LDi_startstopstreaming(client, background);
    LDi_rwlock_wrunlock(&client->clientLock);
}

void
LDClientIdentify(struct LDClient *const client, struct LDUser *const user)
{
    struct LDClient *clientIter, *tmp;

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
clientCloseIsolated(struct LDClient *const client)
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

    LDFree(client);
}

void
LDClientClose(struct LDClient *const client)
{
    struct LDClient *clientIter, *tmp;

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

LDBoolean
LDClientIsInitialized(struct LDClient *const client)
{
    LD_ASSERT(client);
    LDi_rwlock_rdlock(&client->clientLock);
    bool isinit = client->status == LDStatusInitialized;
    LDi_rwlock_rdunlock(&client->clientLock);
    return isinit;
}

LDBoolean
LDClientAwaitInitialized(struct LDClient *const client,
    const unsigned int timeoutmilli)
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
LDClientSaveFlags(struct LDClient *const client)
{
    /* blank for now */
}

void
LDClientRestoreFlags(struct LDClient *const client, const char *const data)
{
    /* blank for now */
}

struct LDJSON *
LDAllFlags(struct LDClient *const client)
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
    struct LDClient *const     client,
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

LDBoolean
LDBoolVariationDetail(struct LDClient *const client, const char *const key,
    LDBoolean fallback, LDVariationDetails *const details)
{
    bool value, *valueRef, fallbackCast;
    struct LDStoreNode *selected;

    LD_ASSERT(client);
    LD_ASSERT(key);

    fallbackCast = fallback;
    valueRef     = &value;

    LDi_rwlock_rdlock(&client->clientLock);
    LDi_evalInternal(
        client, key, LDBool, &fallbackCast, (void **)&valueRef, &selected
    );
    fillDetails(selected, details, LDBool);
    if (selected) {
        LDi_rc_decrement(&selected->rc);
    }
    LDi_rwlock_rdunlock(&client->clientLock);

    return *valueRef;
}

LDBoolean
LDBoolVariation(struct LDClient *const client, const char *const key,
    LDBoolean fallback)
{
    bool value, *valueRef, fallbackCast;

    LD_ASSERT(client);
    LD_ASSERT(key);

    fallbackCast = fallback;
    valueRef     = &value;

    LDi_rwlock_rdlock(&client->clientLock);
    LDi_evalInternal(
        client, key, LDBool, &fallbackCast, (void **)&valueRef, NULL
    );
    LDi_rwlock_rdunlock(&client->clientLock);

    return *valueRef;
}

int
LDIntVariationDetail(struct LDClient *const client, const char *const key,
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
LDIntVariation(struct LDClient *const client, const char *const key,
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
LDDoubleVariationDetail(struct LDClient *const client, const char *const key,
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
LDDoubleVariation(struct LDClient *const client, const char *const key,
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
LDStringVariationDetail(struct LDClient *const client, const char *const key,
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
LDStringVariation(struct LDClient *const client, const char *const key,
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
LDStringVariationAllocDetail(struct LDClient *const client,
    const char *const key, const char* fallback,
    LDVariationDetails *const details)
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
LDStringVariationAlloc(struct LDClient *const client, const char *const key,
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
LDJSONVariationDetail(struct LDClient *const client, const char *const key,
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
LDJSONVariation(struct LDClient *const client, const char *const key,
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
LDClientTrack(struct LDClient *const client, const char *const name)
{
    LD_ASSERT(client);
    LD_ASSERT(name);

    LDi_rwlock_rdlock(&client->shared->sharedUserLock);
    LDi_track(client->eventProcessor, client->shared->sharedUser, name,
        NULL, 0, false);
    LDi_rwlock_rdunlock(&client->shared->sharedUserLock);
}

void
LDClientTrackData(struct LDClient *const client, const char *const name,
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
LDClientTrackMetric(struct LDClient *const client, const char *const name,
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
LDClientFlush(struct LDClient *const client)
{
    struct LDClient *clientIter, *tmp;

    HASH_ITER(hh, globalContext.clientTable, clientIter, tmp) {
        LDi_cond_signal(&clientIter->eventCond);
    }
}

LDBoolean
LDClientRegisterFeatureFlagListener(struct LDClient *const client,
    const char *const key, LDlistenerfn fn)
{
    LD_ASSERT(client);
    LD_ASSERT(key);
    LD_ASSERT(fn);

    return LDi_storeRegisterListener(&client->store, key, fn);
}

void
LDClientUnregisterFeatureFlagListener(struct LDClient *const client,
    const char *const key, LDlistenerfn fn)
{
    LD_ASSERT(client);
    LD_ASSERT(key);
    LD_ASSERT(fn);

    LDi_storeUnregisterListener(&client->store, key, fn);
}

void
LDi_updatestatus(struct LDClient *const client, const LDStatus status)
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
