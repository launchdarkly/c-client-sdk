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
        LDi_rdlock(&client->clientLock);

        const LDStatus status = client->status;

        if (status == LDStatusFailed || finalflush) {
            LDi_log(LD_LOG_TRACE, "killing thread LDi_bgeventsender\n");
            client->threads--;
            LDi_rdunlock(&client->clientLock);
            return THREAD_RETURN_DEFAULT;
        }

        int ms = 30000;
        if (client->config) {
            ms = client->config->eventsFlushIntervalMillis;
        }
        LDi_rdunlock(&client->clientLock);

        if (status != LDStatusShuttingdown) {
            LDi_log(LD_LOG_TRACE, "bg sender sleeping\n");
            LDi_mtxenter(&client->condMtx);
            LDi_condwait(&client->eventCond, &client->condMtx, ms);
            LDi_mtxleave(&client->condMtx);
        }
        LDi_log(LD_LOG_TRACE, "bgsender running\n");

        if (client->status == LDStatusShuttingdown) {
            finalflush = true;
        }

        char *const eventdata = LDi_geteventdata(client);
        if (!eventdata) { continue; }

        LDi_rdlock(&client->clientLock);
        if (client->offline) {
            LDi_rdunlock(&client->clientLock);
            continue;
        }

        bool sent = false;
        int retries = 0;
        while (!sent) {
            LDi_rdunlock(&client->clientLock);
            /* unlocked while sending; will relock if retry needed */
            int response = 0;
            LDi_sendevents(client, eventdata, &response);
            if (response == 401 || response == 403) {
                LDi_wrlock(&client->clientLock);
                LDi_updatestatus(client, LDStatusFailed);
                LDi_wrunlock(&client->clientLock);
                break;
            } else if (response == -1) {
                retries++;
            } else {
                sent = true;
                retries = 0;
            }
            if (retries) {
                unsigned int rng = 0;
                if (!LDi_random(&rng)) {
                    LDi_log(LD_LOG_CRITICAL, "rng failed in bgeventsender\n");
                }

                int backoff = 1000 * pow(2, retries - 2);
                backoff += rng % backoff;
                if (backoff > 3600 * 1000) {
                    backoff = 3600 * 1000;
                    retries--; /* avoid excessive incrementing */
                }

                LDi_millisleep(backoff);
                LDi_rdlock(&client->clientLock);
            }
        }

        free(eventdata);
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
        LDi_rdlock(&client->clientLock);

        if (client->status == LDStatusFailed || client->status == LDStatusShuttingdown) {
            LDi_log(LD_LOG_TRACE, "killing thread LDi_bgfeaturepoller\n");
            client->threads--;
            LDi_rdunlock(&client->clientLock);
            return THREAD_RETURN_DEFAULT;
        }

        /*
         * the logic here is a bit tangled. we start with a default ms poll interval.
         * we skip polling if the client is dead or offline.
         * if we have a config, we revise those values.
         */
        int ms = 3000000;
        bool skippolling = client->offline;
        if (client->config) {
            ms = client->config->pollingIntervalMillis;
            if (client->background) {
                ms = client->config->backgroundPollingIntervalMillis;
                skippolling = skippolling || client->config->disableBackgroundUpdating;
            } else {
                skippolling = skippolling || client->config->streaming;
            }
        }

        /* this triggers the first time the thread runs, so we don't have to wait */
        if (!skippolling && client->status == LDStatusInitializing) { ms = 0; }
        LDi_rdunlock(&client->clientLock);

        if (ms > 0) {
            LDi_mtxenter(&client->condMtx);
            LDi_condwait(&client->pollCond, &client->condMtx, ms);
            LDi_mtxleave(&client->condMtx);
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

        if (response == 401 || response == 403) {
            LDi_wrlock(&client->clientLock);
            LDi_updatestatus(client, LDStatusFailed);
            LDi_wrunlock(&client->clientLock);
        }
        if (!data) { continue; }
        if (LDi_clientsetflags(client, true, data, 1)) {
            LDi_savehash(client);
        }
        free(data);
    }
}

/* exposed for testing */
void
LDi_onstreameventput(LDClient *const client, const char *const data)
{
    if (LDi_clientsetflags(client, true, data, 1)) {
        LDi_savedata("features", client->user->key, data);
    }
}

static void
applypatch(LDClient *const client, cJSON *const payload, const bool isdelete)
{
    LDNode *patch = NULL;
    if (payload->type == cJSON_Object) {
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
                list->fn(node->key, isdelete ? 1 : 0);
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
        LDi_log(LD_LOG_ERROR, "parsing patch failed\n");
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
        LDi_log(LD_LOG_ERROR, "parsing delete patch failed\n");
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

    if (response == 401 || response == 403) {
        LDi_wrlock(&client->clientLock);
        LDi_updatestatus(client, LDStatusFailed);
        LDi_wrunlock(&client->clientLock);
    }
    if (!data) { return; }
    if (LDi_clientsetflags(client, true, data, 1)) {
        LDi_savedata("features", client->user->key, data);
    }
    free(data);
}

static bool shouldstopstreaming;

void
LDi_startstopstreaming(LDClient *const client, bool stopstreaming)
{
    shouldstopstreaming = stopstreaming;
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
    LDi_wrlock(&client->clientLock);
    if (client->streamhandle) {
        LDi_cancelread(client->streamhandle);
        client->streamhandle = 0;
    }
    LDi_condsignal(&client->pollCond);
    LDi_condsignal(&client->streamCond);
    LDi_wrunlock(&client->clientLock);
}

/*
 * as far as event stream parsers go, this is pretty basic.
 * assumes that there's only one line of data following an event identifier line.
 * : -> comment gets eaten
 * event:type -> type is remembered for the next line
 * data:line -> line is processed according to last seen event type
 */
static int
streamcallback(LDClient *const client, const char *line)
{
    LDi_wrlock(&client->clientLock);
    if (*line == ':') {
        LDi_log(LD_LOG_TRACE, "i reject your comment\n");
    } else if (client->wantnewevent) {
        LDi_log(LD_LOG_TRACE, "this better be a new event...\n");
        const char *const eventtype = strchr(line, ':');

        if (!eventtype || eventtype[1] == 0) {
            LDi_log(LD_LOG_TRACE, "unsure in streamcallback\n");
            LDi_wrunlock(&client->clientLock); return 1;
        }

        if (snprintf(client->eventtypebuf, sizeof(client->eventtypebuf), "%s", eventtype + 1) < 0) {
            LDi_log(LD_LOG_ERROR, "snprintf failed in streamcallback type processing\n");
            LDi_wrunlock(&client->clientLock); return 1;
        }

        client->wantnewevent = 0;
    } else if (*line == 0) {
        LDi_log(LD_LOG_TRACE, "end of event\n");
        client->wantnewevent = 1;
    } else {
        if (strncmp(line, "data:", 5) != 0) {
            LDi_log(LD_LOG_ERROR, "not data\n");
            LDi_wrunlock(&client->clientLock); return 1;
        }
        line += 5;
        if (strcmp(client->eventtypebuf, "put") == 0) {
            LDi_wrunlock(&client->clientLock);
            LDi_log(LD_LOG_TRACE, "PUT\n");
            LDi_onstreameventput(client, line);
            LDi_wrlock(&client->clientLock);
        } else if (strcmp(client->eventtypebuf, "patch") == 0) {
            LDi_wrunlock(&client->clientLock);
            LDi_log(LD_LOG_TRACE, "PATCH\n");
            LDi_onstreameventpatch(client, line);
            LDi_wrlock(&client->clientLock);
        } else if (strcmp(client->eventtypebuf, "delete") == 0) {
            LDi_wrunlock(&client->clientLock);
            LDi_log(LD_LOG_TRACE, "DELETE\n");
            LDi_onstreameventdelete(client, line);
            LDi_wrlock(&client->clientLock);
        } else if (strcmp(client->eventtypebuf, "ping") == 0) {
            LDi_wrunlock(&client->clientLock);
            LDi_log(LD_LOG_TRACE, "PING\n");
            onstreameventping(client);
            LDi_wrlock(&client->clientLock);
        }
    }

    LDi_wrunlock(&client->clientLock);

    if (shouldstopstreaming) { return 1; }

    return 0;
}

THREAD_RETURN
LDi_bgfeaturestreamer(void *const v)
{
    LDClient *const client = v;

    int retries = 0;
    while (true) {
        LDi_rdlock(&client->clientLock);

        if (client->status == LDStatusFailed || client->status == LDStatusShuttingdown) {
            LDi_log(LD_LOG_TRACE, "killing thread LDi_bgfeaturestreamer\n");
            client->threads--;
            LDi_rdunlock(&client->clientLock);
            return THREAD_RETURN_DEFAULT;
        }

        if (!client->config->streaming || client->offline || client->background) {
            LDi_rdunlock(&client->clientLock);
            int ms = 30000;
            LDi_mtxenter(&client->condMtx);
            LDi_condwait(&client->streamCond, &client->condMtx, ms);
            LDi_mtxleave(&client->condMtx);
            continue;
        }

        LDi_rdunlock(&client->clientLock);

        int response;
        /* this won't return until it disconnects */
        LDi_readstream(client,  &response, streamcallback, LDi_updatehandle);

        if (response == 401 || response == 403) {
            LDi_wrlock(&client->clientLock);
            LDi_updatestatus(client, LDStatusFailed);
            LDi_wrunlock(&client->clientLock);
            continue;
        } else if (response == -1) {
            retries++;
        } else {
            retries = 0;
        }
        if (retries) {
            unsigned int rng = 0;
            if (!LDi_random(&rng)) {
                LDi_log(LD_LOG_CRITICAL, "rng failed in bgeventsender\n");
            }

            int backoff = 1000 * pow(2, retries - 2);
            backoff += rng % backoff;
            if (backoff > 3600 * 1000) {
                backoff = 3600 * 1000;
                retries--; /* avoid excessive incrementing */
            }

            LDi_millisleep(backoff);
        }
    }
}
