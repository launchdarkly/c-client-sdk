#include <stdio.h>
#include <stdlib.h>
#ifndef _WINDOWS
#include <unistd.h>
#else
#endif
#include <math.h>

#include <launchdarkly/api.h>
#include <launchdarkly/json.h>

#include <curl/curl.h>

#include "flag.h"
#include "ldinternal.h"

/*
 * all the code that runs in the background here.
 * plus the server event parser and streaming update handler.
 */

THREAD_RETURN
LDi_bgeventsender(void *const v)
{
    struct LDClient *const client     = v;
    LDBoolean              finalflush = LDBooleanFalse;

    while (LDBooleanTrue) {
        struct LDJSON *payloadJSON;
        char *         payloadSerialized;
        LDStatus       status;
        int            ms;
        char           payloadId[LD_UUID_SIZE + 1];
        LDBoolean      sendFailed;

        LDi_rwlock_wrlock(&client->clientLock);

        status = client->status;

        if (status == LDStatusFailed || finalflush) {
            LD_LOG(LD_LOG_TRACE, "killing thread LDi_bgeventsender");
            LDi_rwlock_wrunlock(&client->clientLock);
            return THREAD_RETURN_DEFAULT;
        }

        ms = client->shared->sharedConfig->eventsFlushIntervalMillis;

        if (status != LDStatusShuttingdown) {
            LDi_mutex_lock(&client->condMtx);
            LDi_rwlock_wrunlock(&client->clientLock);
            LDi_cond_wait(&client->eventCond, &client->condMtx, ms);
            LDi_mutex_unlock(&client->condMtx);
        } else {
            LDi_rwlock_wrunlock(&client->clientLock);
        }

        LD_LOG(LD_LOG_TRACE, "bgsender running");

        LDi_rwlock_rdlock(&client->clientLock);
        if (client->status == LDStatusShuttingdown) {
            finalflush = LDBooleanTrue;
        }

        if (client->offline) {
            LDi_rwlock_rdunlock(&client->clientLock);
            continue;
        }
        LDi_rwlock_rdunlock(&client->clientLock);

        payloadId[LD_UUID_SIZE] = 0;

        if (!LDi_UUIDv4(payloadId)) {
            LD_LOG(LD_LOG_ERROR, "failed to generate payload identifier");

            continue;
        }

        if (!LDi_bundleEventPayload(client->eventProcessor, &payloadJSON)) {
            LD_LOG(
                LD_LOG_ERROR,
                "LDi_bgeventsender failed to bundle event payload");

            continue;
        }

        if (payloadJSON == NULL) {
            continue;
        }

        if (!(payloadSerialized = LDJSONSerialize(payloadJSON))) {
            LD_LOG(
                LD_LOG_ERROR,
                "LDi_bgeventsender failed to serialize event payload");

            LDJSONFree(payloadJSON);

            continue;
        }

        LDJSONFree(payloadJSON);

        sendFailed = LDBooleanFalse;
        while (LDBooleanTrue) {
            long response = 0;

            LDi_sendevents(client, payloadSerialized, payloadId, &response);

            if (response == 200 || response == 202) {
                LD_LOG(LD_LOG_TRACE, "successfuly sent event batch");

                sendFailed = LDBooleanFalse;

                break;
            } else {
                if (sendFailed == LDBooleanTrue) {
                    break;
                }

                sendFailed = LDBooleanTrue;

                if (response == 401 || response == 403) {
                    LDi_rwlock_wrlock(&client->clientLock);
                    LDi_updatestatus(client, LDStatusFailed);
                    LDi_rwlock_wrunlock(&client->clientLock);

                    LD_LOG(
                        LD_LOG_ERROR,
                        "mobile key not authorized, event sending failed");

                    break;
                }

                LDi_mutex_lock(&client->condMtx);
                LDi_cond_wait(&client->eventCond, &client->condMtx, 1000);
                LDi_mutex_unlock(&client->condMtx);
            }
        }

        if (sendFailed) {
            LD_LOG(
                LD_LOG_WARNING, "sending events failed deleting event batch");
        }

        LDFree(payloadSerialized);
    }
}

/*
 * this thread always runs, even when using streaming, but then it just sleeps
 */
THREAD_RETURN
LDi_bgfeaturepoller(void *const v)
{
    struct LDClient *const client = v;

    while (LDBooleanTrue) {
        LDBoolean skippolling;
        int       ms;
        long      response;
        char *    data;

        LDi_rwlock_wrlock(&client->clientLock);

        if (client->status == LDStatusFailed ||
            client->status == LDStatusShuttingdown) {
            LD_LOG(LD_LOG_TRACE, "killing thread LDi_bgfeaturepoller");
            LDi_rwlock_wrunlock(&client->clientLock);
            return THREAD_RETURN_DEFAULT;
        }

        skippolling = client->offline;
        ms          = client->shared->sharedConfig->pollingIntervalMillis;
        if (client->background) {
            ms = client->shared->sharedConfig->backgroundPollingIntervalMillis;
            skippolling =
                skippolling ||
                client->shared->sharedConfig->disableBackgroundUpdating;
        } else {
            skippolling =
                skippolling || client->shared->sharedConfig->streaming;
        }

        /* this triggers the first time the thread runs, so we don't have
        to wait */
        if (!skippolling && client->status == LDStatusInitializing) {
            ms = 0;
        }

        if (ms > 0) {
            LDi_mutex_lock(&client->condMtx);
            LDi_rwlock_wrunlock(&client->clientLock);
            LDi_cond_wait(&client->pollCond, &client->condMtx, ms);
            LDi_mutex_unlock(&client->condMtx);
        } else {
            LDi_rwlock_wrunlock(&client->clientLock);
        }

        if (skippolling) {
            continue;
        }

        LDi_rwlock_rdlock(&client->clientLock);
        if (client->status == LDStatusFailed ||
            client->status == LDStatusShuttingdown) {
            LDi_rwlock_rdunlock(&client->clientLock);
            continue;
        }
        LDi_rwlock_rdunlock(&client->clientLock);

        response = 0;
        data     = LDi_fetchfeaturemap(client, &response);

        if (response == 200) {
            if (!data) {
                continue;
            }

            LDi_onstreameventput(client, data);
        } else if (response == 401 || response == 403) {
            LDi_rwlock_wrlock(&client->clientLock);
            LDi_updatestatus(client, LDStatusFailed);
            LDi_rwlock_wrunlock(&client->clientLock);

            LD_LOG(LD_LOG_ERROR, "mobile key not authorized, polling failed");
        } else {
            LD_LOG(LD_LOG_ERROR, "poll failed will retry again");
        }

        LDFree(data);
    }
}

LDBoolean
LDi_onstreameventput(struct LDClient *const client, const char *const data)
{
    struct LDJSON *payload, *payloadIter;
    struct LDFlag *flags;
    size_t flagCount;
    size_t i;
    LDBoolean storeResult;

    payload = NULL;

    if (!(payload = LDJSONDeserialize(data))) {
        LD_LOG(LD_LOG_ERROR, "stream PUT: failed to deserialize event data");
        return LDBooleanFalse;
    }

    if (LDJSONGetType(payload) != LDObject) {
        LD_LOG(LD_LOG_ERROR, "stream PUT: data must be a JSON object");
        LDJSONFree(payload);
        return LDBooleanFalse;
    }

    flagCount = LDCollectionGetSize(payload);

    if (!(flags = LDAlloc(sizeof(struct LDFlag) * flagCount))) {
        LD_LOG(LD_LOG_ERROR, "stream PUT: failed to allocate space for flags");
        LDJSONFree(payload);
        return LDBooleanFalse;
    }

    payloadIter = LDGetIter(payload);

    for (i = 0; i < flagCount; i++)  {
        LD_ASSERT(payloadIter);
        if (!LDi_flag_parse(&flags[i], LDIterKey(payloadIter), payloadIter)) {
            size_t j;

            LD_LOG(LD_LOG_ERROR, "stream PUT: error parsing flag");
            for (j = 0; j < i; i++) {
                LDi_flag_destroy(&flags[i]);
            }

            LDFree(payload);
            LDFree(flags);
            return LDBooleanFalse;
        }
        payloadIter = LDIterNext(payloadIter);
    }

    LDFree(payload);

    storeResult = LDi_storePut(&client->store, flags, flagCount);

    LDi_rwlock_wrlock(&client->clientLock);
    LDi_updatestatus(client, storeResult ? LDStatusInitialized : LDStatusFailed);
    LDi_rwlock_wrunlock(&client->clientLock);

    return storeResult;
}

void
LDi_onstreameventpatch(struct LDClient *const client, const char *const data)
{
    struct LDJSON *payload;
    struct LDFlag  flag;

    LD_ASSERT(client);
    LD_ASSERT(data);

    payload = NULL;

    if (!(payload = LDJSONDeserialize(data))) {
        LD_LOG(LD_LOG_ERROR, "failed to deserialize patch discarding update");

        goto cleanup;
    }

    if (!LDi_flag_parse(&flag, NULL, payload)) {
        LD_LOG(LD_LOG_ERROR, "failed to parse flag patch discarding update");

        goto cleanup;
    }

    if (!LDi_storeUpsert(&client->store, flag)) {
        LD_LOG(LD_LOG_ERROR, "failed to upsert flag");

        goto cleanup;
    }

cleanup:
    LDJSONFree(payload);
}

void
LDi_onstreameventdelete(struct LDClient *const client, const char *const data)
{
    struct LDJSON *payload, *tmp;
    const char *   key;
    unsigned int   version;

    LD_ASSERT(client);
    LD_ASSERT(data);

    payload = NULL;

    if (!(payload = LDJSONDeserialize(data))) {
        LD_LOG(LD_LOG_ERROR, "failed to parse delete discarding update");

        goto cleanup;
    }

    if (LDJSONGetType(payload) != LDObject) {
        goto cleanup;
    }

    if (!(tmp = LDObjectLookup(payload, "version"))) {
        goto cleanup;
    }

    if (LDJSONGetType(tmp) != LDNumber) {
        goto cleanup;
    }

    version = LDGetNumber(tmp);

    if (!(tmp = LDObjectLookup(payload, "key"))) {
        goto cleanup;
    }

    if (LDJSONGetType(tmp) != LDText) {
        goto cleanup;
    }

    key = LDGetText(tmp);

    if (!LDi_storeDelete(&client->store, key, version)) {
        LD_LOG(LD_LOG_ERROR, "failed to delete flag");

        goto cleanup;
    }

cleanup:
    LDJSONFree(payload);
}

void
LDi_startstopstreaming(
    struct LDClient *const client, const LDBoolean stopstreaming)
{
    client->shouldstopstreaming = stopstreaming;
    LDi_cond_signal(&client->pollCond);
    LDi_cond_signal(&client->streamCond);
}

static void
LDi_updatehandle(struct LDClient *const client, const int handle)
{
    LDi_rwlock_wrlock(&client->clientLock);
    LDi_socketStore(&client->streamhandle, handle);
    LDi_rwlock_wrunlock(&client->clientLock);
}

void
LDi_reinitializeconnection(struct LDClient *const client)
{
    LDBoolean socketOpen;
    int socketHandle;

    socketOpen = LDi_socketLoad(&client->streamhandle, &socketHandle);

    if (socketOpen) {
        LDi_cancelread(socketHandle);
        LDi_socketClose(&client->streamhandle);
    }
    LDi_cond_signal(&client->pollCond);
    LDi_cond_signal(&client->streamCond);
}

static LDBoolean
LDi_onEvent(
    const char *const eventName,
    const char *const eventBuffer,
    void *const       rawContext)
{
    struct LDClient *client;

    LD_ASSERT(eventName);
    LD_ASSERT(eventBuffer);
    LD_ASSERT(rawContext);

    client = (struct LDClient *)rawContext;

    if (strcmp(eventName, "put") == 0) {
        LDi_onstreameventput(client, eventBuffer);
    } else if (strcmp(eventName, "patch") == 0) {
        LDi_onstreameventpatch(client, eventBuffer);
    } else if (strcmp(eventName, "delete") == 0) {
        LDi_onstreameventdelete(client, eventBuffer);
    } else {
        LD_LOG_1(LD_LOG_ERROR, "sse unknown event name: %s", eventName);
    }

    return LDBooleanTrue;
}

double
LDi_calculateStreamDelay(const unsigned int retries)
{
    if (retries == 0) {
        return 0;
    } else if (retries == 1) {
        return 1000;
    } else {
        double backoff;

        unsigned int rng = 0;

        LDi_random(&rng);

        /* calculate time to wait */
        backoff = 1000 * pow(2, retries);

        /* cap (min not built in) */
        if (backoff > 30 * 1000) {
            backoff = 30 * 1000;
        }

        /* jitter */
        backoff /= 2;

        return backoff + LDi_normalize(rng, 0, LD_RAND_MAX, 0, backoff);
    }
}

THREAD_RETURN
LDi_bgfeaturestreamer(void *const v)
{
    struct LDClient *const client = v;

    int retries = 0;

    while (LDBooleanTrue) {
        time_t    startedOn;
        long       response;
        LDBoolean intentionallyClosed;

        /* Wait on any retry delays required. Status change such as shut down
        will cause a short circuit */
        if (retries) {
            LDi_mutex_lock(&client->condMtx);
            LDi_cond_wait(
                &client->streamCond,
                &client->condMtx,
                LDi_calculateStreamDelay(retries));
            LDi_mutex_unlock(&client->condMtx);
        }

        LDi_rwlock_wrlock(&client->clientLock);

        /* Handle shutdown if initialized */
        if (client->status == LDStatusFailed ||
            client->status == LDStatusShuttingdown) {
            LD_LOG(LD_LOG_TRACE, "killing thread LDi_bgfeaturestreamer");

            LDi_rwlock_wrunlock(&client->clientLock);

            return THREAD_RETURN_DEFAULT;
        }

        /* If we are actually not supposed to be streaming just wait */
        if (!client->shared->sharedConfig->streaming || client->offline ||
            client->background)
        {
            /* Ensures we skip directly to shutdown handler */
            retries = 0;

            LDi_mutex_lock(&client->condMtx);
            LDi_rwlock_wrunlock(&client->clientLock);
            LDi_cond_wait(&client->streamCond, &client->condMtx, 1000);
            LDi_mutex_unlock(&client->condMtx);

            continue;
        }

        LDi_rwlock_wrunlock(&client->clientLock);

        startedOn = time(NULL);

        {
            struct LDSSEParser parser;

            LDSSEParserInitialize(&parser, LDi_onEvent, (void *)client);

            /* this won't return until it disconnects */
            LDi_readstream(client, &response, &parser, LDi_updatehandle);

            LDSSEParserDestroy(&parser);
        }

        if (response == CURLE_COULDNT_RESOLVE_HOST) {
            LD_LOG(LD_LOG_ERROR, "couldn't resolve host for streaming endpoint");

        } else if (response >= 400 && response < 500) {
            LDBoolean permanentFailure = LDBooleanFalse;

            if (response == 401 || response == 403) {
                LD_LOG(
                    LD_LOG_ERROR,
                    "mobile key not authorized, streaming failed");

                permanentFailure = LDBooleanTrue;
            } else if (response != 400 && response != 408 && response != 429) {
                LD_LOG(LD_LOG_ERROR, "streaming unrecoverable response code");

                permanentFailure = LDBooleanTrue;
            }

            if (permanentFailure) {
                LDi_rwlock_wrlock(&client->clientLock);
                LDi_updatestatus(client, LDStatusFailed);
                LDi_rwlock_wrunlock(&client->clientLock);

                LD_LOG(LD_LOG_TRACE, "streaming permanent failure");

                return THREAD_RETURN_DEFAULT;
            }
        }

        LDi_rwlock_rdlock(&client->clientLock);
        intentionallyClosed = LDi_socketClosed(&client->streamhandle);
        LDi_rwlock_rdunlock(&client->clientLock);

        if (intentionallyClosed) {
            retries = 0;
        } else {
            if (response == 200) {
                if (time(NULL) > startedOn + 60) {
                    LD_LOG(
                        LD_LOG_ERROR,
                        "streaming failed after 60 seconds, retrying");

                    retries = 0;
                } else {
                    LD_LOG(
                        LD_LOG_ERROR,
                        "streaming failed within 60 seconds, backing off");

                    retries++;
                }
            } else {
                LD_LOG(
                    LD_LOG_ERROR,
                    "streaming failed with recoverable error, backing off");

                retries++;
            }
        }
    }
}
