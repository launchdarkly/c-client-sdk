#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include "ldapi.h"
#include "ldinternal.h"

/*
 * all the code that runs in the background here.
 * plus the server event parser and streaming update handler.
 */

pthread_t LDi_eventthread;
pthread_t LDi_pollingthread;
pthread_t LDi_streamingthread;
pthread_cond_t LDi_bgeventcond = PTHREAD_COND_INITIALIZER;


static void *
bgeventsender(void *v)
{
    LDClient *client = v;
    pthread_mutex_t dummymtx = PTHREAD_MUTEX_INITIALIZER;

    while (true) {
        LDi_rdlock(&LDi_clientlock);
        int ms = 30000;
        if (client->config)
            ms = client->config->eventsFlushIntervalMillis;
        LDi_unlock(&LDi_clientlock);

        LDi_log(20, "bg sender sleeping\n");
        LDi_mtxenter(&dummymtx);
        LDi_condwait(&LDi_bgeventcond, &dummymtx, ms);
        LDi_mtxleave(&dummymtx);
        LDi_log(20, "bgsender running\n");

        char *eventdata = LDi_geteventdata();
        if (!eventdata) {
            LDi_log(20, "no event data to send\n");
            continue;
        }

        LDi_rdlock(&LDi_clientlock);
        if (client->dead || client->offline) {
            LDi_unlock(&LDi_clientlock);
            continue;
        }
        
        bool sent = false;
        int retries = 0;
        while (!sent) {
            char url[4096];
            snprintf(url, sizeof(url), "%s/mobile", client->config->eventsURI);
            char authkey[256];
            snprintf(authkey, sizeof(authkey), "%s", client->config->mobileKey);
            LDi_unlock(&LDi_clientlock);
            /* unlocked while sending; will relock if retry needed */
            int response = 0;
            LDi_sendevents(url, authkey, eventdata, &response);
            if (response == 401 || response == 403) {
                sent = true; /* consider it done */
                client->dead = true;
                retries = 0;
                if (LDi_statuscallback)
                    LDi_statuscallback(0);
            } else if (response == -1) {
                retries++;
            } else {
                sent = true;
                retries = 0;
            }
            if (retries) {
                int backoff = 1000 * pow(2, retries - 2);
                backoff += random() % backoff;
                if (backoff > 3600 * 1000) {
                    backoff = 3600 * 1000;
                    retries--; /* avoid excessive incrementing */
                }
                LDi_millisleep(backoff);
                LDi_rdlock(&LDi_clientlock);
            }
        }
        free(eventdata);
    }
}

/*
 * this thread always runs, even when using streaming, but then it just sleeps
 */
static void *
bgfeaturepoller(void *v)
{
    LDClient *client = v;
    char *prevdata = NULL;

    while (true) {
        LDi_rdlock(&LDi_clientlock);
        int ms = 3000000;
        if (client->config)
            ms = client->config->pollingIntervalMillis;
        bool skippolling = client->dead || client->config->streaming || client->offline;
        if (!skippolling && !client->isinit)
            ms = 0;
        LDi_unlock(&LDi_clientlock);

        LDi_log(20, "bg poller sleeping\n");
        if (ms > 0)
            LDi_millisleep(ms);
        if (skippolling) {
            continue;
        }
        LDi_log(20, "bg poller running\n");

        LDi_rdlock(&LDi_clientlock);
        if (client->dead) {
            LDi_unlock(&LDi_clientlock);
            continue;
        }
        
        char *userurl = LDi_usertourl(client->user);
        char url[4096];
        snprintf(url, sizeof(url), "%s/msdk/eval/users/%s", client->config->appURI, userurl);
        free(userurl);
        char authkey[256];
        snprintf(authkey, sizeof(authkey), "%s", client->config->mobileKey);

        LDi_unlock(&LDi_clientlock);
        
        int response = 0;
        char *data = LDi_fetchfeaturemap(url, authkey, &response);
        if (response == 401 || response == 403) {
            client->dead = true;
            if (LDi_statuscallback)
                    LDi_statuscallback(0);
        }
        if (!data)
            continue;
        if (LDi_clientsetflags(client, true, data, 0)) {
            LDi_savehash(client);
        }
        free(data);
    }
}

/* exposed for testing */
void
LDi_onstreameventput(const char *data)
{
    LDClient *client = LDClientGet();
    if (LDi_clientsetflags(client, true, data, 1)) {
        LDi_savedata("features", client->user->key, data);
    }
}

static void
applypatch(cJSON *payload, bool isdelete)
{
    LDNode *patch = NULL;
    if (payload->type == cJSON_Object) {
        patch = LDi_jsontohash(payload, 2);
    }
    cJSON_Delete(payload);

    LDClient *client = LDClientGet();
    LDi_wrlock(&LDi_clientlock);
    LDNode *hash = client->allFlags;
    LDNode *node, *tmp;
    HASH_ITER(hh, patch, node, tmp) {
        LDNode *res = NULL;
        HASH_FIND_STR(hash, node->key, res);
        if (res && res->version > node->version) {
            /* stale patch, skip */
            LDi_log(10, "patch for %s version %d is staler than %d\n", node->key, node->version, res->version);
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
    LDi_unlock(&LDi_clientlock);

    LDi_freehash(patch);
}

void
LDi_onstreameventpatch(const char *data)
{
    cJSON *payload = cJSON_Parse(data);

    if (!payload) {
        LDi_log(5, "parsing patch failed\n");
        return;
    }
    applypatch(payload, false);
    LDi_savehash(LDClientGet());
}

void
LDi_onstreameventdelete(const char *data)
{
    cJSON *payload = cJSON_Parse(data);

    if (!payload) {
        LDi_log(5, "parsing delete patch failed\n");
        return;
    }
    applypatch(payload, 1);
    LDi_savehash(LDClientGet());
}

static void
onstreameventping(void)
{
    LDClient *client = LDClientGet();

    LDi_rdlock(&LDi_clientlock);
    if (client->dead) {
        LDi_unlock(&LDi_clientlock);
        return;
    }
    char *userurl = LDi_usertourl(client->user);
    char url[4096];
    snprintf(url, sizeof(url), "%s/msdk/eval/users/%s", client->config->appURI, userurl);
    free(userurl);
    char authkey[256];
    snprintf(authkey, sizeof(authkey), "%s", client->config->mobileKey);

    LDi_unlock(&LDi_clientlock);

    int response = 0;
    char *data = LDi_fetchfeaturemap(url, authkey, &response);
    if (response == 401 || response == 403) {
        client->dead = true;
        if (LDi_statuscallback)
            LDi_statuscallback(0);
    }
    if (!data)
        return;
    if (LDi_clientsetflags(client, true, data, 1)) {
        LDi_savedata("features", client->user->key, data);
    }
    free(data);
}

/*
 * as far as event stream parsers go, this is pretty basic.
 * assumes that there's only one line of data following an event identifier line.
 * : -> comment gets eaten
 * event:type -> type is remembered for the next line
 * data:line -> line is processed according to last seen event type
 */
static int
streamcallback(const char *line)
{
    static int wantnewevent = 1;
    static char eventtypebuf[256];

    if (*line == ':') {
        LDi_log(10, "i reject your comment\n");
    } else if (wantnewevent) {
        LDi_log(15, "this better be a new event...\n");
        char *eventtype = strchr(line, ':');
        if (!eventtype || eventtype[1] == 0) {
            LDi_log(5, "unsure\n");
            return 1;
        }
        snprintf(eventtypebuf, sizeof(eventtypebuf), "%s", eventtype + 1);
        wantnewevent = 0;
    } else if (*line == 0) {
        LDi_log(15, "end of event\n");
        wantnewevent = 1;
    } else {
        if (strncmp(line, "data:", 5) != 0) {
            LDi_log(5, "not data\n");
            return 1;
        }
        line += 5;
        if (strcmp(eventtypebuf, "put") == 0) {
            LDi_log(15, "PUT\n");
            LDi_onstreameventput(line);
        } else if (strcmp(eventtypebuf, "patch") == 0) {
            LDi_log(15, "PATCH\n");
            LDi_onstreameventpatch(line);
        } else if (strcmp(eventtypebuf, "delete") == 0) {
            LDi_log(15, "DELETE\n");
            LDi_onstreameventdelete(line);
        } else if (strcmp(eventtypebuf, "ping") == 0) {
            LDi_log(15, "PING\n");
            onstreameventping();
        }
        LDi_log(100, "here is data for the event %s\n", eventtypebuf);
        LDi_log(100, "the data: %s\n", line);
    }
    return 0;
}

static void *
bgfeaturestreamer(void *v)
{
    LDClient *client = v;

    int retries = 0;
    while (true) {
        LDi_rdlock(&LDi_clientlock);
        if (client->dead || !client->config->streaming || client->offline) {
            LDi_unlock(&LDi_clientlock);
            LDi_millisleep(30000);
            continue;
        }
        char *userurl = LDi_usertourl(client->user);

        char url[4096];
        snprintf(url, sizeof(url), "%s/meval/%s", client->config->streamURI, userurl);
        free(userurl);
        
        char authkey[256];
        snprintf(authkey, sizeof(authkey), "%s", client->config->mobileKey);

        LDi_unlock(&LDi_clientlock);

        int response;
        /* this won't return until it disconnects */
        LDi_readstream(url, authkey, &response, streamcallback);
        if (response == 401 || response == 403) {
                client->dead = true;
                retries = 0;
                if (LDi_statuscallback)
                    LDi_statuscallback(0);
        } else if (response == -1) {
                retries++;
        } else {
                retries = 0;
        }
        if (retries) {
            int backoff = 1000 * pow(2, retries - 2);
            backoff += random() % backoff;
            if (backoff > 3600 * 1000) {
                backoff = 3600 * 1000;
                retries--; /* avoid excessive incrementing */
            }
            LDi_millisleep(backoff);
            LDi_rdlock(&LDi_clientlock);
        }
    }
}

void
LDi_startthreads(LDClient *client)
{
    pthread_create(&LDi_eventthread, NULL, bgeventsender, client);
    pthread_create(&LDi_pollingthread, NULL, bgfeaturepoller, client);
    pthread_create(&LDi_streamingthread, NULL, bgfeaturestreamer, client);
}