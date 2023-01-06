#include <stdio.h>
#include <stdlib.h>

#ifndef _WINDOWS
#include <unistd.h>
#endif

#include <math.h>

#include <curl/curl.h>

#include <launchdarkly/api.h>

#include "ldinternal.h"
#include "uthash.h"

#define UNUSED(x) (void)(x)

static struct LDGlobal_i globalContext;

ld_once_t LDi_earlyonce = LD_ONCE_INIT;

/** Support for non-userdata LDSetClientStatusCallback function. */
static void (*LDi_statuscallback)(int) = NULL;

/** Support for userdata-accepting LDSetClientStatusCallbackUserData function. */
struct status_callback_closure {
    LDstatusfn callback;
    void *userData;
};

/** Allows the deprecated non-userdata accepting callback to be treated as one accepting userdata. */
static void status_callback_wrapper(LDStatus status, void *userData) {
    UNUSED(userData);
    LDi_statuscallback(status);
}

/** Stores a userdata-accepting status callback, along with the userdata payload itself. */
static struct status_callback_closure LDi_statuscallback_closure = {
        NULL,
        NULL
};

void
LDi_earlyinit(void)
{
    LDi_rwlock_init(&globalContext.sharedUserLock);
    globalContext.clientTable   = NULL;
    globalContext.primaryClient = NULL;
    globalContext.sharedConfig  = NULL;
    globalContext.sharedUser    = NULL;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    LDi_initializerng();
}

struct LDClient *
LDClientGet(void)
{
    return globalContext.primaryClient;
}

struct LDClient *
LDClientGetForMobileKey(const char *const keyName)
{
    struct LDClient *lookup;

    LD_ASSERT_API(keyName);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (keyName == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientGetForMobileKey NULL keyName");

        return NULL;
    }
#endif

    HASH_FIND_STR(globalContext.clientTable, keyName, lookup);

    return lookup;
}

struct LDClient *
LDi_clientInitIsolated(
    struct LDGlobal_i *const shared, const char *const mobileKey)
{
    struct LDClient *client;
    unsigned int     threadCount;

    LD_ASSERT_API(shared);
    LD_ASSERT_API(mobileKey);

    threadCount = 0;

    LDi_once(&LDi_earlyonce, LDi_earlyinit);

    if (!(client = LDAlloc(sizeof(*client)))) {
        LD_LOG(LD_LOG_CRITICAL, "no memory for the client");

        return NULL;
    }

    memset(client, 0, sizeof(*client));

    client->shared              = shared;
    client->offline             = shared->sharedConfig->offline;
    client->background          = LDBooleanFalse;
    client->status              = LDStatusInitializing;
    client->shouldstopstreaming = LDBooleanFalse;

    LDi_initSocket(&client->streamhandle);

    if (!LDSetString(&client->mobileKey, mobileKey)) {
        goto err1;
    }

    if (!(client->eventProcessor = LDi_newEventProcessor(shared->sharedConfig)))
    {
        goto err2;
    }

    if (!LDi_storeInitialize(&client->store)) {
        goto err3;
    }

    if (!LDi_rwlock_init(&client->clientLock)) {
        goto err4;
    }

    if (!LDi_mutex_init(&client->initCondMtx)) {
        goto err5;
    }

    if (!LDi_mutex_init(&client->condMtx)) {
        goto err6;
    }

    if (!LDi_cond_init(&client->initCond)) {
        goto err7;
    }

    if (!LDi_cond_init(&client->eventCond)) {
        goto err8;
    }

    if (!LDi_cond_init(&client->pollCond)) {
        goto err9;
    }

    if (!LDi_cond_init(&client->streamCond)) {
        goto err10;
    }

    if (!LDi_thread_create(&client->eventThread, LDi_bgeventsender, client)) {
        goto err11;
    }
    threadCount++;

    if (!LDi_thread_create(&client->pollingThread, LDi_bgfeaturepoller, client))
    {
        goto err12;
    }
    threadCount++;

    if (!LDi_thread_create(
            &client->streamingThread, LDi_bgfeaturestreamer, client)) {
        goto err12;
    }
    threadCount++;

    LDi_rwlock_rdlock(&shared->sharedUserLock);

    if (!LDi_identify(client->eventProcessor, shared->sharedUser)) {
        LDi_rwlock_rdunlock(&shared->sharedUserLock);

        goto err12;
    }

    LDi_rwlock_rdunlock(&shared->sharedUserLock);

    return client;

err12:
    LDi_rwlock_wrlock(&client->clientLock);
    LDi_updatestatus(client, LDStatusShuttingdown);
    LDi_reinitializeconnection(client);
    LDi_rwlock_wrunlock(&client->clientLock);

    if (threadCount > 0) {
        LDi_thread_join(&client->eventThread);
    }

    if (threadCount > 1) {
        LDi_thread_join(&client->pollingThread);
    }

    if (threadCount > 2) {
        LDi_thread_join(&client->streamingThread);
    }
err11:
    LDi_cond_destroy(&client->streamCond);
err10:
    LDi_cond_destroy(&client->pollCond);
err9:
    LDi_cond_destroy(&client->eventCond);
err8:
    LDi_cond_destroy(&client->initCond);
err7:
    LDi_mutex_destroy(&client->condMtx);
err6:
    LDi_mutex_destroy(&client->initCondMtx);
err5:
    LDi_rwlock_destroy(&client->clientLock);
err4:
    LDi_storeDestroy(&client->store);
err3:
    LDi_freeEventProcessor(client->eventProcessor);
err2:
    LDFree(client->mobileKey);
err1:
    LDFree(client);

    return NULL;
}

struct LDClient *
LDClientInit(
    struct LDConfig *const config,
    struct LDUser *const   user,
    const unsigned int     maxwaitmilli)
{
    struct LDJSON *secondaryKey;

    LD_ASSERT_API(config);
    LD_ASSERT_API(user);
    LD_ASSERT_API(!globalContext.primaryClient);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientInit NULL config");

        return NULL;
    }

    if (user == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientInit NULL user");

        return NULL;
    }

    if (globalContext.primaryClient != NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientInit already initialized");

        return NULL;
    }
#endif

    globalContext.sharedUser   = user;
    globalContext.sharedConfig = config;

    globalContext.primaryClient =
        LDi_clientInitIsolated(&globalContext, config->mobileKey);

    LD_ASSERT(globalContext.primaryClient);

    HASH_ADD_KEYPTR(
        hh,
        globalContext.clientTable,
        LDPrimaryEnvironmentName,
        strlen(LDPrimaryEnvironmentName),
        globalContext.primaryClient);

    for (secondaryKey = LDGetIter(config->secondaryMobileKeys); secondaryKey;
         secondaryKey = LDIterNext(secondaryKey))
    {
        const char *     name;
        struct LDClient *secondaryClient;

        name = LDIterKey(secondaryKey);

        LD_ASSERT(name);

        secondaryClient =
            LDi_clientInitIsolated(&globalContext, LDGetText(secondaryKey));

        LD_ASSERT(secondaryClient);

        HASH_ADD_KEYPTR(
            hh, globalContext.clientTable, name, strlen(name), secondaryClient);
    }

    if (maxwaitmilli) {
        struct LDClient *clientIter, *clientTmp;

        const double future = 1000 * (double)time(NULL) + maxwaitmilli;

        HASH_ITER(hh, globalContext.clientTable, clientIter, clientTmp)
        {
            const double now = 1000 * (double)time(NULL);

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

    LD_ASSERT_API(client);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientSetOffline NULL client");

        return;
    }
#endif

    HASH_ITER(hh, globalContext.clientTable, clientIter, tmp)
    {
        LDi_rwlock_wrlock(&clientIter->clientLock);
        clientIter->offline = LDBooleanTrue;
        LDi_rwlock_wrunlock(&clientIter->clientLock);
    }
}

void
LDClientSetOnline(struct LDClient *const client)
{
    struct LDClient *clientIter, *tmp;

    LD_ASSERT_API(client);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientSetOnline NULL client");

        return;
    }
#endif

    HASH_ITER(hh, globalContext.clientTable, clientIter, tmp)
    {
        LDi_rwlock_wrlock(&clientIter->clientLock);
        clientIter->offline = LDBooleanFalse;
        LDi_updatestatus(clientIter, LDStatusInitializing);
        LDi_rwlock_wrunlock(&clientIter->clientLock);
    }
}

LDBoolean
LDClientIsOffline(struct LDClient *const client)
{
    LDBoolean offline;

    LD_ASSERT_API(client);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientIsOffline NULL client");

        return LDBooleanTrue;
    }
#endif

    LDi_rwlock_rdlock(&client->clientLock);
    offline = client->offline;
    LDi_rwlock_rdunlock(&client->clientLock);

    return offline;
}

void
LDClientSetBackground(struct LDClient *const client, const LDBoolean background)
{
    LD_ASSERT_API(client);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientSetBackground NULL client");

        return;
    }
#endif

    LDi_rwlock_wrlock(&client->clientLock);
    client->background = background;
    LDi_startstopstreaming(client, background);
    LDi_rwlock_wrunlock(&client->clientLock);
}

void
LDClientIdentify(struct LDClient *const client, struct LDUser *const user)
{
    struct LDClient *clientIter, *tmp;
    struct LDUser *  previousUser;
    LDBoolean        shouldAlias;

    LD_ASSERT_API(client);
    LD_ASSERT_API(user);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientIdentify NULL client");

        return;
    }

    if (user == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientIdentify NULL user");

        return;
    }
#endif

    LDi_rwlock_wrlock(&globalContext.sharedUserLock);

    previousUser             = globalContext.sharedUser;
    globalContext.sharedUser = user;
    shouldAlias              = previousUser->anonymous && !user->anonymous &&
                  !globalContext.sharedConfig->autoAliasOptOut;

    HASH_ITER(hh, globalContext.clientTable, clientIter, tmp)
    {
        LDi_rwlock_wrlock(&clientIter->clientLock);

        LDi_updatestatus(client, LDStatusInitializing);

        LDi_reinitializeconnection(clientIter);
        LDi_identify(clientIter->eventProcessor, user);

        if (shouldAlias) {
            LDi_alias(clientIter->eventProcessor, user, previousUser);
        }

        LDi_rwlock_wrunlock(&clientIter->clientLock);
    }

    if (previousUser != user) {
        LDUserFree(previousUser);
    }

    LDi_rwlock_wrunlock(&globalContext.sharedUserLock);
}

void
clientCloseIsolated(struct LDClient *const client)
{
    LD_ASSERT_API(client);

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
    if (client) {
        struct LDClient *clientIter, *tmp;

        HASH_ITER(hh, globalContext.clientTable, clientIter, tmp)
        {
            HASH_DEL(globalContext.clientTable, clientIter);
            clientCloseIsolated(clientIter);
        }

        LDUserFree(globalContext.sharedUser);
        LDConfigFree(globalContext.sharedConfig);

        globalContext.sharedConfig  = NULL;
        globalContext.primaryClient = NULL;
        globalContext.clientTable   = NULL;
    }
}

LDBoolean
LDClientIsInitialized(struct LDClient *const client)
{
    LDBoolean isInit;

    LD_ASSERT_API(client);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientIsInitialized NULL client");

        return LDBooleanFalse;
    }
#endif

    LDi_rwlock_rdlock(&client->clientLock);
    isInit = client->status == LDStatusInitialized;
    LDi_rwlock_rdunlock(&client->clientLock);

    return isInit;
}

LDBoolean
LDClientAwaitInitialized(
    struct LDClient *const client, const unsigned int timeoutmilli)
{
    LDBoolean isInit;

    LD_ASSERT_API(client);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientAwaitInitialized NULL client");

        return LDBooleanFalse;
    }
#endif

    LDi_mutex_lock(&client->initCondMtx);
    LDi_rwlock_rdlock(&client->clientLock);

    if (client->status == LDStatusInitialized) {
        LDi_rwlock_rdunlock(&client->clientLock);
        LDi_mutex_unlock(&client->initCondMtx);

        return LDBooleanTrue;
    }

    LDi_rwlock_rdunlock(&client->clientLock);

    LDi_cond_wait(&client->initCond, &client->initCondMtx, timeoutmilli);
    LDi_mutex_unlock(&client->initCondMtx);

    LDi_rwlock_rdlock(&client->clientLock);
    isInit = client->status == LDStatusInitialized;
    LDi_rwlock_rdunlock(&client->clientLock);

    return isInit;
}

void
LDSetClientStatusCallback(void(callback)(int))
{
    if (callback == NULL) {
        LDSetClientStatusCallbackUserData(NULL, NULL);
    } else {
        LDi_statuscallback = callback;
        LDSetClientStatusCallbackUserData(status_callback_wrapper, NULL);
    }
}

void
LDSetClientStatusCallbackUserData(LDstatusfn callback, void *userData)
{
    LDi_statuscallback_closure.callback = callback;
    LDi_statuscallback_closure.userData = userData;
}


char *
LDClientSaveFlags(struct LDClient *const client)
{
    struct LDJSON *bundle;
    char *         serialized;

    LD_ASSERT_API(client);

    if (!(bundle = LDi_storeGetJSON(&client->store))) {
        return NULL;
    }

    if (!(serialized = LDJSONSerialize(bundle))) {
        LDJSONFree(bundle);

        return NULL;
    }

    LDJSONFree(bundle);

    return serialized;
}

LDBoolean
LDClientRestoreFlags(struct LDClient *const client, const char *const data)
{
    LD_ASSERT_API(client);
    LD_ASSERT_API(data);

    return LDi_onstreameventput(client, data);
}

struct LDJSON *
LDAllFlags(struct LDClient *const client)
{
    struct LDJSON *      result;
    struct LDStoreNode **flags;
    unsigned int         flagCount, i;

    LD_ASSERT_API(client);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientAllFlags NULL client");

        return LDBooleanFalse;
    }
#endif

    if (!(result = LDNewObject())) {
        return NULL;
    }

    if (!LDi_storeGetAll(&client->store, &flags, &flagCount)) {
        LDJSONFree(result);

        return NULL;
    }

    for (i = 0; i < flagCount; i++) {
        struct LDJSON *tmp;

        if (!flags[i]->flag.deleted)
        {
            if (!(tmp = LDJSONDuplicate(flags[i]->flag.value)))
            {
                goto error;
            }

            if (!(LDObjectSetKey(result, flags[i]->flag.key, tmp)))
            {
                LDJSONFree(tmp);

                goto error;
            }
        }

        LDi_rc_decrement(&flags[i]->rc);
    }

    LDFree(flags);

    return result;

error:
    for (; i < flagCount; i++) {
        LDi_rc_decrement(&flags[i]->rc);
    }

    LDFree(flags);
    LDFree(result);

    return NULL;
}

static void
fillDetails(
    const struct LDClient *const    client,
    const char *const               flagKey,
    const struct LDStoreNode *const node,
    LDVariationDetails *const       details,
    const LDJSONType                type)
{
    LD_ASSERT(details);

    if (!client) {
        details->reason         = LDNewObject();
        details->variationIndex = -1;

        LDObjectSetKey(details->reason, "kind", LDNewText("ERROR"));
        LDObjectSetKey(
            details->reason, "errorKind", LDNewText("CLIENT_NOT_SPECIFIED"));
    } else if (!flagKey) {
        details->reason         = LDNewObject();
        details->variationIndex = -1;

        LDObjectSetKey(details->reason, "kind", LDNewText("ERROR"));
        LDObjectSetKey(
            details->reason, "errorKind", LDNewText("FLAG_NOT_SPECIFIED"));
    } else if (node) {
        if (type == LDNull || LDJSONGetType(node->flag.value) == type ||
            LDJSONGetType(node->flag.value) == LDNull)
        {
            if (node->flag.reason) {
                details->reason = LDJSONDuplicate(node->flag.reason);
            } else {
                details->reason = NULL;
            }

            details->variationIndex = node->flag.variation;
        } else {
            details->reason         = LDNewObject();
            details->variationIndex = -1;

            LDObjectSetKey(details->reason, "kind", LDNewText("ERROR"));
            LDObjectSetKey(
                details->reason, "errorKind", LDNewText("WRONG_TYPE"));
        }
    } else {
        details->reason         = LDNewObject();
        details->variationIndex = -1;

        LDObjectSetKey(details->reason, "kind", LDNewText("ERROR"));
        LDObjectSetKey(
            details->reason, "errorKind", LDNewText("FLAG_NOT_FOUND"));
    }
}

static void
LDi_castJSONToValue(void **const destination, struct LDJSON *const source)
{
    LD_ASSERT(destination);
    LD_ASSERT(source);

    switch (LDJSONGetType(source)) {
    case LDNull:
        LD_ASSERT(LDBooleanFalse);
        break;

    case LDBool:
        **((LDBoolean * *const) destination) = LDGetBool(source);
        break;

    case LDText:
        *((const char **const)destination) = LDGetText(source);
        break;

    case LDNumber:
        **((double **const)destination) = LDGetNumber(source);
        break;

    case LDObject:
        LD_ASSERT(LDBooleanFalse);
        break;

    case LDArray:
        LD_ASSERT(LDBooleanFalse);
        break;
    }
}

static LDBoolean
LDi_evalInternal(
    struct LDClient *const     client,
    const char *const          flagKey,
    const LDJSONType           variationKind,
    void *const                fallbackValue,
    void **const               resultValue,
    struct LDStoreNode **const selected)
{
    struct LDStoreNode *node;

    LD_ASSERT_API(client);
    LD_ASSERT_API(flagKey);
    LD_ASSERT_API(fallbackValue);
    LD_ASSERT(resultValue);

    if (selected) {
        *selected = NULL;
    }

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDi_evalInternal NULL client");

        *resultValue = fallbackValue;

        return LDBooleanFalse;
    }

    if (flagKey == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDi_evalInternal NULL flagKey");

        *resultValue = fallbackValue;

        return LDBooleanFalse;
    }
#endif

    node = LDi_storeGet(&client->store, flagKey);

    if (node && (variationKind == LDNull ||
                 LDJSONGetType(node->flag.value) == variationKind))
    {
        if (variationKind == LDNull) {
            *((struct LDJSON * *const) resultValue) = node->flag.value;
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
        selected != NULL);

    LDi_rwlock_rdunlock(&client->shared->sharedUserLock);

    if (selected) {
        *selected = node;
    } else if (node) {
        LDi_rc_decrement(&node->rc);
    }

    return LDBooleanTrue;
}

LDBoolean
LDBoolVariationDetail(
    struct LDClient *const    client,
    const char *const         key,
    const LDBoolean           fallback,
    LDVariationDetails *const details)
{
    LDBoolean           value, *valueRef, fallbackCast;
    struct LDStoreNode *selected;

    LD_ASSERT_API(client);
    LD_ASSERT_API(key);

    fallbackCast = fallback;
    valueRef     = &value;

    LDi_evalInternal(
        client, key, LDBool, &fallbackCast, (void **)&valueRef, &selected);
    fillDetails(client, key, selected, details, LDBool);
    if (selected) {
        LDi_rc_decrement(&selected->rc);
    }

    return *valueRef;
}

LDBoolean
LDBoolVariation(
    struct LDClient *const client,
    const char *const      key,
    const LDBoolean        fallback)
{
    LDBoolean value, *valueRef, fallbackCast;

    LD_ASSERT_API(client);
    LD_ASSERT_API(key);

    fallbackCast = fallback;
    valueRef     = &value;

    LDi_evalInternal(
        client, key, LDBool, &fallbackCast, (void **)&valueRef, NULL);

    return *valueRef;
}

int
LDIntVariationDetail(
    struct LDClient *const    client,
    const char *const         key,
    const int                 fallback,
    LDVariationDetails *const details)
{
    double              value, *valueRef, fallbackCast;
    struct LDStoreNode *selected;

    LD_ASSERT_API(client);
    LD_ASSERT_API(key);

    valueRef     = &value;
    fallbackCast = fallback;

    LDi_evalInternal(
        client, key, LDNumber, &fallbackCast, (void **)&valueRef, &selected);
    fillDetails(client, key, selected, details, LDNumber);
    if (selected) {
        LDi_rc_decrement(&selected->rc);
    }

    return *valueRef;
}

int
LDIntVariation(
    struct LDClient *const client, const char *const key, const int fallback)
{
    double value, *valueRef, fallbackCast;

    LD_ASSERT_API(client);
    LD_ASSERT_API(key);

    valueRef     = &value;
    fallbackCast = fallback;

    LDi_evalInternal(
        client, key, LDNumber, &fallbackCast, (void **)&valueRef, NULL);

    return *valueRef;
}

double
LDDoubleVariationDetail(
    struct LDClient *const    client,
    const char *const         key,
    const double              fallback,
    LDVariationDetails *const details)
{
    double              value, *valueRef, fallbackCast;
    struct LDStoreNode *selected;

    LD_ASSERT_API(client);
    LD_ASSERT_API(key);

    valueRef     = &value;
    fallbackCast = fallback;

    LDi_evalInternal(
        client, key, LDNumber, &fallbackCast, (void **)&valueRef, &selected);
    fillDetails(client, key, selected, details, LDNumber);
    if (selected) {
        LDi_rc_decrement(&selected->rc);
    }

    return *valueRef;
}

double
LDDoubleVariation(
    struct LDClient *const client, const char *const key, const double fallback)
{
    double value, *valueRef, fallbackCast;

    LD_ASSERT_API(client);
    LD_ASSERT_API(key);

    valueRef     = &value;
    fallbackCast = fallback;

    LDi_evalInternal(
        client, key, LDNumber, &fallbackCast, (void **)&valueRef, NULL);

    return *valueRef;
}

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

char *
LDStringVariationDetail(
    struct LDClient *const    client,
    const char *const         key,
    const char *const         fallback,
    char *const               buffer,
    const size_t              bufferSize,
    LDVariationDetails *const details)
{
    size_t resultLength;
    char *value = NULL;
    struct LDStoreNode *selected = NULL;

    LD_ASSERT_API(client);
    LD_ASSERT_API(key);
    LD_ASSERT_API(!(!buffer && bufferSize));

    LDi_evalInternal(
        client, key, LDText, (void *)fallback, (void **)&value, &selected);
    fillDetails(client, key, selected, details, LDText);
    if (selected) {
        LDi_rc_decrement(&selected->rc);
    }

    resultLength = min(strlen(value), bufferSize - 1);
    memcpy(buffer, value, resultLength);
    buffer[resultLength] = '\0';

    return buffer;
}

char *
LDStringVariation(
    struct LDClient *const client,
    const char *const      key,
    const char *const      fallback,
    char *const            buffer,
    const size_t           bufferSize)
{
    size_t resultLength;
    char *value = NULL;

    LD_ASSERT_API(client);
    LD_ASSERT_API(key);
    LD_ASSERT_API(!(!buffer && bufferSize));

    LDi_evalInternal(
        client, key, LDText, (void *)fallback, (void **)&value, NULL);

    resultLength = min(strlen(value), bufferSize - 1);
    memcpy(buffer, value, resultLength);
    buffer[resultLength] = '\0';

    return buffer;
}

char *
LDStringVariationAllocDetail(
    struct LDClient *const    client,
    const char *const         key,
    const char *const         fallback,
    LDVariationDetails *const details)
{
    char *value = NULL;
    struct LDStoreNode *selected = NULL;

    LD_ASSERT_API(client);
    LD_ASSERT_API(key);
    LD_ASSERT_API(fallback);

    LDi_evalInternal(
        client, key, LDText, (void *)fallback, (void **)&value, &selected);
    fillDetails(client, key, selected, details, LDText);
    if (selected) {
        LDi_rc_decrement(&selected->rc);
    }

    return LDStrDup(value);
}

char *
LDStringVariationAlloc(
    struct LDClient *const client,
    const char *const      key,
    const char *const      fallback)
{
    char *value = NULL;

    LD_ASSERT_API(client);
    LD_ASSERT_API(key);
    LD_ASSERT_API(fallback);

    LDi_evalInternal(
        client, key, LDText, (void *)fallback, (void **)&value, NULL);

    return LDStrDup(value);
}

struct LDJSON *
LDJSONVariationDetail(
    struct LDClient *const     client,
    const char *const          key,
    const struct LDJSON *const fallback,
    LDVariationDetails *const  details)
{
    const struct LDJSON *value;
    struct LDStoreNode * selected;

    LD_ASSERT_API(client);
    LD_ASSERT_API(key);
    LD_ASSERT_API(fallback);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDJSONVariationDetail NULL client");

        return LDJSONDuplicate(fallback);
    }

    if (key == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDJSONVariationDetail NULL key");

        return LDJSONDuplicate(fallback);
    }
#endif

    LDi_evalInternal(
        client, key, LDNull, (void *)fallback, (void **)&value, &selected);
    fillDetails(client, key, selected, details, LDNull);
    if (selected) {
        LDi_rc_decrement(&selected->rc);
    }

    return LDJSONDuplicate(value);
}

struct LDJSON *
LDJSONVariation(
    struct LDClient *const     client,
    const char *const          key,
    const struct LDJSON *const fallback)
{
    const struct LDJSON *value;

    LD_ASSERT_API(client);
    LD_ASSERT_API(key);
    LD_ASSERT_API(fallback);

    LDi_evalInternal(
        client, key, LDNull, (void *)fallback, (void **)&value, NULL);

    return LDJSONDuplicate(value);
}

void
LDClientAlias(
    struct LDClient *const     client,
    const struct LDUser *const currentUser,
    const struct LDUser *const previousUser)
{
    LD_ASSERT_API(client);
    LD_ASSERT_API(currentUser);
    LD_ASSERT_API(previousUser);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientAlias NULL client");

        return;
    }

    if (currentUser == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientAlias NULL currentUser");

        return;
    }

    if (previousUser == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientAlias NULL previousUser");

        return;
    }
#endif

    LDi_alias(client->eventProcessor, currentUser, previousUser);
}

void
LDClientTrack(struct LDClient *const client, const char *const name)
{
    LD_ASSERT_API(client);
    LD_ASSERT_API(name);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientTrack NULL client");

        return;
    }

    if (name == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientTrack NULL name");

        return;
    }
#endif

    LDi_rwlock_rdlock(&client->shared->sharedUserLock);
    LDi_track(
        client->eventProcessor,
        client->shared->sharedUser,
        name,
        NULL,
        0,
        LDBooleanFalse);
    LDi_rwlock_rdunlock(&client->shared->sharedUserLock);
}

void
LDClientTrackData(
    struct LDClient *const client,
    const char *const      name,
    struct LDJSON *const   data)
{
    LD_ASSERT_API(client);
    LD_ASSERT_API(name);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientTrackData NULL client");

        return;
    }

    if (name == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientTrackData NULL name");

        return;
    }
#endif

    LDi_rwlock_rdlock(&client->shared->sharedUserLock);
    LDi_track(
        client->eventProcessor,
        client->shared->sharedUser,
        name,
        data,
        0,
        LDBooleanFalse);
    LDi_rwlock_rdunlock(&client->shared->sharedUserLock);
}

void
LDClientTrackMetric(
    struct LDClient *const client,
    const char *const      name,
    struct LDJSON *const   data,
    const double           metric)
{
    LD_ASSERT_API(client);
    LD_ASSERT_API(name);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientTrackMetric NULL client");

        return;
    }

    if (name == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientTrackMetric NULL name");

        return;
    }
#endif

    LDi_rwlock_rdlock(&client->shared->sharedUserLock);
    LDi_track(
        client->eventProcessor,
        client->shared->sharedUser,
        name,
        data,
        metric,
        LDBooleanTrue);
    LDi_rwlock_rdunlock(&client->shared->sharedUserLock);
}

void
LDClientFlush(struct LDClient *const client)
{
    struct LDClient *clientIter, *tmp;

    LD_ASSERT_API(client);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientFlush NULL client");

        return;
    }
#endif

    HASH_ITER(hh, globalContext.clientTable, clientIter, tmp)
    {
        LDi_cond_signal(&clientIter->eventCond);
    }
}

LDBoolean
LDClientRegisterFeatureFlagListener(
    struct LDClient *const client, const char *const key, LDlistenerfn fn)
{
    LD_ASSERT_API(client);
    LD_ASSERT_API(key);
    LD_ASSERT_API(fn);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(
            LD_LOG_WARNING, "LDClientRegisterFeatureFlagListener NULL client");

        return LDBooleanFalse;
    }

    if (key == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDClientRegisterFeatureFlagListener NULL key");

        return LDBooleanFalse;
    }

    if (fn == NULL) {
        LD_LOG(
            LD_LOG_WARNING,
            "LDClientRegisterFeatureFlagListener NULL listener");

        return LDBooleanFalse;
    }
#endif

    return LDi_storeRegisterListener(&client->store, key, fn);
}

void
LDClientUnregisterFeatureFlagListener(
    struct LDClient *const client, const char *const key, LDlistenerfn fn)
{
    LD_ASSERT_API(client);
    LD_ASSERT_API(key);
    LD_ASSERT_API(fn);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (client == NULL) {
        LD_LOG(
            LD_LOG_WARNING,
            "LDClientUnregisterFeatureFlagListener NULL client");

        return;
    }

    if (key == NULL) {
        LD_LOG(
            LD_LOG_WARNING, "LDClientUnregisterFeatureFlagListener NULL key");

        return;
    }

    if (fn == NULL) {
        LD_LOG(
            LD_LOG_WARNING,
            "LDClientUnregisterFeatureFlagListener NULL listener");

        return;
    }
#endif

    LDi_storeUnregisterListener(&client->store, key, fn);
}

void
LDi_updatestatus(struct LDClient *const client, const LDStatus status)
{
    if (client->status != status) {
        client->status = status;
        if (LDi_statuscallback_closure.callback) {
            LDi_rwlock_wrunlock(&client->clientLock);
            LDi_statuscallback_closure.callback(status, LDi_statuscallback_closure.userData);
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
