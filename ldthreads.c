#include <stdlib.h>
#include <stdio.h>
#ifndef _WINDOWS
#include <unistd.h>
#else
#endif
#include <math.h>

#include "ldapi.h"
#include "ldinternal.h"

/*
 * all the code that runs in the background here.
 * plus the server event parser and streaming update handler.
 */

THREAD_RETURN
LDi_bgeventsender(void *const v)
{
    LDClient *const client = v; bool finalflush = false;

    while (true) {
        LDi_wrlock(&client->clientLock);

        const LDStatus status = client->status;

        if (status == LDStatusFailed || finalflush) {
            LDi_log(LD_LOG_TRACE, "killing thread LDi_bgeventsender");
            LDi_wrunlock(&client->clientLock);
            return THREAD_RETURN_DEFAULT;
        }

        const int ms = client->shared->sharedConfig->eventsFlushIntervalMillis;

        if (status != LDStatusShuttingdown) {
            LDi_mtxenter(&client->condMtx);
            LDi_wrunlock(&client->clientLock);
            LDi_condwait(&client->eventCond, &client->condMtx, ms);
            LDi_mtxleave(&client->condMtx);
        } else {
            LDi_wrunlock(&client->clientLock);
        }

        LDi_log(LD_LOG_TRACE, "bgsender running");

        LDi_rdlock(&client->clientLock);
        if (client->status == LDStatusShuttingdown) {
            finalflush = true;
        }

        if (client->offline) {
            LDi_rdunlock(&client->clientLock);
            continue;
        }
        LDi_rdunlock(&client->clientLock);

        char payloadId[LD_UUID_SIZE + 1];
        payloadId[LD_UUID_SIZE] = 0;

        if (!LDi_UUIDv4(payloadId)) {
            LDi_log(LD_LOG_ERROR, "failed to generate payload identifier");

            continue;
        }

        char *const eventdata = LDi_geteventdata(client);
        if (!eventdata) { continue; }

        bool sendFailed = false;
        while (true) {
            int response = 0;

            LDi_sendevents(client, eventdata, payloadId, &response);

            if (response == 200 || response == 202) {
                LDi_log(LD_LOG_TRACE, "successfuly sent event batch");

                sendFailed = false;

                break;
            } else {
                if (sendFailed == true) {
                    break;
                }

                sendFailed = true;

                if (response == 401 || response == 403) {
                    LDi_wrlock(&client->clientLock);
                    LDi_updatestatus(client, LDStatusFailed);
                    LDi_wrunlock(&client->clientLock);

                    LDi_log(LD_LOG_ERROR, "mobile key not authorized, event sending failed");

                    break;
                }

                LDi_mtxenter(&client->condMtx);
                LDi_condwait(&client->eventCond, &client->condMtx, 1000);
                LDi_mtxleave(&client->condMtx);
            }
        }

        if (sendFailed) {
            LDi_log(LD_LOG_WARNING, "sending events failed deleting event batch");
        }

        LDFree(eventdata);
    }
}

/*
 * this thread always runs, even when using streaming, but then it just sleeps
 */
THREAD_RETURN
LDi_bgfeaturepoller(void *const v)
{
    LDClient *const client = v;

    while (true) {
        LDi_wrlock(&client->clientLock);

        if (client->status == LDStatusFailed || client->status == LDStatusShuttingdown) {
            LDi_log(LD_LOG_TRACE, "killing thread LDi_bgfeaturepoller");
            LDi_wrunlock(&client->clientLock);
            return THREAD_RETURN_DEFAULT;
        }

        bool skippolling = client->offline;
        int ms = client->shared->sharedConfig->pollingIntervalMillis;
        if (client->background) {
            ms = client->shared->sharedConfig->backgroundPollingIntervalMillis;
            skippolling = skippolling || client->shared->sharedConfig->disableBackgroundUpdating;
        } else {
            skippolling = skippolling || client->shared->sharedConfig->streaming;
        }

        /* this triggers the first time the thread runs, so we don't have to wait */
        if (!skippolling && client->status == LDStatusInitializing) { ms = 0; }

        if (ms > 0) {
            LDi_mtxenter(&client->condMtx);
            LDi_wrunlock(&client->clientLock);
            LDi_condwait(&client->pollCond, &client->condMtx, ms);
            LDi_mtxleave(&client->condMtx);
        } else {
            LDi_wrunlock(&client->clientLock);
        }

        if (skippolling) { continue; }

        LDi_rdlock(&client->clientLock);
        if (client->status == LDStatusFailed || client->status == LDStatusShuttingdown) {
            LDi_rdunlock(&client->clientLock);
            continue;
        }
        LDi_rdunlock(&client->clientLock);

        int response = 0;
        char *const data = LDi_fetchfeaturemap(client, &response);

        if (response == 200) {
            if (!data) { continue; }

            if (LDi_clientsetflags(client, true, data, 1)) {
                LDi_savehash(client);
            }
        } else if (response == 401 || response == 403) {
            LDi_wrlock(&client->clientLock);
            LDi_updatestatus(client, LDStatusFailed);
            LDi_wrunlock(&client->clientLock);

            LDi_log(LD_LOG_ERROR, "mobile key not authorized, polling failed");
        } else {
            LDi_log(LD_LOG_ERROR, "poll failed will retry again");
        }

        LDFree(data);
    }
}

/* exposed for testing */
void
LDi_onstreameventput(LDClient *const client, const char *const data)
{
    if (LDi_clientsetflags(client, true, data, 1)) {
        LDi_rdlock(&client->shared->sharedUserLock);
        LDi_savedata("features", client->shared->sharedUser->key, data);
        LDi_rdunlock(&client->shared->sharedUserLock);
    }
}

static void
applypatch(LDClient *const client, cJSON *const payload, const bool isdelete)
{
    LDNode *patch = NULL;
    if (cJSON_IsObject(payload)) {
        patch = LDi_jsontohash(payload, 2);
    }
    cJSON_Delete(payload);

    LDi_wrlock(&client->clientLock);
    LDNode *hash = client->allFlags;
    LDNode *node, *tmp;
    HASH_ITER(hh, patch, node, tmp) {
        LDNode *res = NULL;
        HASH_FIND_STR(hash, node->key, res);
        if (res && res->version > node->version) {
            /* stale patch, skip */
            continue;
        }
        if (res) {
            HASH_DEL(hash, res);
            LDi_freenode(res);
        }
        if (!isdelete) {
            HASH_DEL(patch, node);
            HASH_ADD_KEYPTR(hh, hash, node->key, strlen(node->key), node);
        }
        for (struct listener *list = client->listeners; list; list = list->next) {
            if (strcmp(list->key, node->key) == 0) {
                LDi_wrunlock(&client->clientLock);
                list->fn(node->key, isdelete ? 1 : 0);
                LDi_wrlock(&client->clientLock);
            }
        }
    }

    client->allFlags = hash;
    LDi_wrunlock(&client->clientLock);

    LDi_freehash(patch);
}

void
LDi_onstreameventpatch(LDClient *const client, const char *const data)
{
    cJSON *const payload = cJSON_Parse(data);

    if (!payload) {
        LDi_log(LD_LOG_ERROR, "parsing patch failed");
        return;
    }

    applypatch(client, payload, false);
    LDi_savehash(client);
}

void
LDi_onstreameventdelete(LDClient *const client, const char *const data)
{
    cJSON *const payload = cJSON_Parse(data);

    if (!payload) {
        LDi_log(LD_LOG_ERROR, "parsing delete patch failed");
        return;
    }

    applypatch(client, payload, 1);
    LDi_savehash(client);
}

static void
onstreameventping(LDClient *const client)
{
    LDi_rdlock(&client->clientLock);

    if (client->status == LDStatusFailed || client->status == LDStatusShuttingdown) {
        LDi_rdunlock(&client->clientLock);
        return;
    }

    LDi_rdunlock(&client->clientLock);

    int response = 0;
    char *const data = LDi_fetchfeaturemap(client, &response);

    if (response == 200) {
        if (!data) { return; }

        if (LDi_clientsetflags(client, true, data, 1)) {
            LDi_rdlock(&client->shared->sharedUserLock);
            LDi_savedata("features", client->shared->sharedUser->key, data);
            LDi_rdunlock(&client->shared->sharedUserLock);
        }
    } else {
        if (response == 401 || response == 403) {
            LDi_wrlock(&client->clientLock);
            LDi_updatestatus(client, LDStatusFailed);
            LDi_wrunlock(&client->clientLock);
        }
    }

    LDFree(data);
}

void
LDi_startstopstreaming(LDClient *const client, bool stopstreaming)
{
    client->shouldstopstreaming = stopstreaming;
    LDi_condsignal(&client->pollCond);
    LDi_condsignal(&client->streamCond);
}

static void
LDi_updatehandle(LDClient *const client, const int handle)
{
    LDi_wrlock(&client->clientLock);
    client->streamhandle = handle;
    LDi_wrunlock(&client->clientLock);
}

void
LDi_reinitializeconnection(LDClient *const client)
{
    if (client->streamhandle) {
        LDi_cancelread(client->streamhandle);
        client->streamhandle = 0;
    }
    LDi_condsignal(&client->pollCond);
    LDi_condsignal(&client->streamCond);
}

/*
 * as far as event stream parsers go, this is pretty basic.
 * : -> comment gets eaten
 * event:type -> type is remembered for the next data lines
 * data:line -> line is processed according to last seen event type
 */
static int
streamcallback(LDClient *const client, const char *line)
{
    LDi_wrlock(&client->clientLock);

    if (client->shouldstopstreaming) {
        free(client->databuffer); client->databuffer = NULL; client->eventname[0] = 0;
        LDi_wrunlock(&client->clientLock);
        return 1;
    }

    if (!line) {
        //should not happen from the networking side but is not fatal
        LDi_log(LD_LOG_ERROR, "streamcallback got NULL line");
    } else if (line[0] == ':') {
        //skip comment
    } else if (line[0] == 0) {
        if (client->eventname[0] == 0) {
            LDi_log(LD_LOG_WARNING, "streamcallback got dispatch but type was never set");
        } else if (strcmp(client->eventname, "ping") == 0) {
            LDi_wrunlock(&client->clientLock);
            onstreameventping(client);
            LDi_wrlock(&client->clientLock);
        } else if (client->databuffer == NULL) {
            LDi_log(LD_LOG_WARNING, "streamcallback got dispatch but data was never set");
        } else if (strcmp(client->eventname, "put") == 0) {
            LDi_wrunlock(&client->clientLock);
            LDi_onstreameventput(client, client->databuffer);
            LDi_wrlock(&client->clientLock);
        } else if (strcmp(client->eventname, "patch") == 0) {
            LDi_wrunlock(&client->clientLock);
            LDi_onstreameventpatch(client, client->databuffer);
            LDi_wrlock(&client->clientLock);
        } else if (strcmp(client->eventname, "delete") == 0) {
            LDi_wrunlock(&client->clientLock);
            LDi_onstreameventdelete(client, client->databuffer);
            LDi_wrlock(&client->clientLock);
        } else {
            LDi_log(LD_LOG_WARNING, "streamcallback unknown event name: %s", client->eventname);
        }

        free(client->databuffer); client->databuffer = NULL; client->eventname[0] = 0;
    } else if (strncmp(line, "data:", 5) == 0) {
        line += 5; line += line[0] == ' ';

        const bool nempty = client->databuffer != NULL;

        const size_t linesize = strlen(line);

        size_t currentsize = 0;
        if (nempty) { currentsize = strlen(client->databuffer); }

        client->databuffer = realloc(client->databuffer, linesize + currentsize + nempty + 1);

        if (nempty) { client->databuffer[currentsize] = '\n'; }

        memcpy(client->databuffer + currentsize + nempty, line, linesize);

        client->databuffer[currentsize + nempty + linesize] = 0;
    } else if (strncmp(line, "event:", 6) == 0) {
        line += 6; line += line[0] == ' ';

        if (snprintf(client->eventname, sizeof(client->eventname), "%s", line) < 0) {
            LDi_wrunlock(&client->clientLock);
            LDi_log(LD_LOG_CRITICAL, "snprintf failed in streamcallback type processing");
            return 1;
        }
    }

    LDi_wrunlock(&client->clientLock);

    return 0;
}

THREAD_RETURN
LDi_bgfeaturestreamer(void *const v)
{
    LDClient *const client = v;

    int retries = 0;

    while (true) {
        /* Wait on any retry delays required. Status change such as shut down
        will cause a short circuit */
        if (retries) {
            int delay = 0;

            if (retries == 1) {
                delay = 1000;
            } else {
                unsigned int rng = 0;

                LD_ASSERT(LDi_random(&rng));

                delay = 1000 * pow(2, retries - 2);

                delay += rng % delay;

                if (delay > 30 * 1000) {
                    delay = 30 * 1000;
                }
            }

            LDi_mtxenter(&client->condMtx);
            LDi_condwait(&client->streamCond, &client->condMtx, delay);
            LDi_mtxleave(&client->condMtx);
        }

        LDi_wrlock(&client->clientLock);

        /* Handle shutdown if initialized */
        if (client->status == LDStatusFailed
            || client->status == LDStatusShuttingdown)
        {
            LDi_log(LD_LOG_TRACE, "killing thread LDi_bgfeaturestreamer");

            LDi_wrunlock(&client->clientLock);

            return THREAD_RETURN_DEFAULT;
        }

        /* If we are actually not supposed to be streaming just wait */
        if (!client->shared->sharedConfig->streaming || client->offline
            || client->background)
        {
            /* Ensures we skip directly to shutdown handler */
            retries = 0;

            LDi_mtxenter(&client->condMtx);
            LDi_wrunlock(&client->clientLock);
            LDi_condwait(&client->streamCond, &client->condMtx, 1000);
            LDi_mtxleave(&client->condMtx);

            continue;
        }

        LDi_wrunlock(&client->clientLock);

        const time_t startedOn = time(NULL);

        int response;
        /* this won't return until it disconnects */
        LDi_readstream(client,  &response, streamcallback, LDi_updatehandle);

        if (response >= 400 && response < 500) {
            bool permanentFailure = false;

            if (response == 401 || response == 403) {
                LDi_log(LD_LOG_ERROR,
                    "mobile key not authorized, streaming failed");

                permanentFailure = true;
            } else if (response != 400 && response != 408 && response != 429) {
                LDi_log(LD_LOG_ERROR, "streaming unrecoverable response code");

                permanentFailure = true;
            }

            if (permanentFailure) {
                LDi_wrlock(&client->clientLock);
                LDi_updatestatus(client, LDStatusFailed);
                LDi_wrunlock(&client->clientLock);

                LDi_log(LD_LOG_TRACE, "streaming permanent failure");

                return THREAD_RETURN_DEFAULT;
            }
        }

        LDi_rdlock(&client->clientLock);
        const bool intentionallyClosed = client->streamhandle != 0;
        LDi_rdunlock(&client->clientLock);

        if (response == 200 && intentionallyClosed) {
            retries = 0;
        } else {
            if (response == 200) {
                if (time(NULL) > startedOn + 60) {
                    LDi_log(LD_LOG_ERROR,
                        "streaming failed after 60 seconds, retrying");

                    retries = 0;
                } else {
                    LDi_log(LD_LOG_ERROR,
                        "streaming failed within 60 seconds, backing off");

                    retries++;
                }
            } else {
                LDi_log(LD_LOG_ERROR,
                    "streaming failed with recoverable error, backing off");

                retries++;
            }
        }
    }
}
