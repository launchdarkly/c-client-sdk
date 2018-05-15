#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include <curl/curl.h>

#include "ldapi.h"
#include "ldinternal.h"


static pthread_once_t clientonce = PTHREAD_ONCE_INIT;


static LDClient *theClient;
pthread_rwlock_t LDi_clientlock = PTHREAD_RWLOCK_INITIALIZER;

void (*LDi_statuscallback)(int);

LDConfig *
LDConfigNew(const char *mobileKey)
{
    LDConfig *config;

    config = LDAlloc(sizeof(*config));
    if (!config) {
        LDi_log(2, "no memory for config\n");
        return NULL;
    }
    memset(config, 0, sizeof(*config));
    config->allAttributesPrivate = false;
    config->backgroundPollingIntervalMillis = 3600000;
    LDSetString(&config->appURI, "https://app.launchdarkly.com");
    config->connectionTimeoutMillis = 10000;
    config->disableBackgroundUpdating = false;
    config->eventsCapacity = 100;
    config->eventsFlushIntervalMillis = 30000;
    LDSetString(&config->eventsURI, "https://mobile.launchdarkly.com");
    LDSetString(&config->mobileKey, mobileKey);
    config->offline = false;
    config->pollingIntervalMillis = 300000;
    config->privateAttributeNames = NULL;
    config->streaming = true;
    LDSetString(&config->streamURI, "https://clientstream.launchdarkly.com");
    config->useReport = false;

    return config;
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


LDUser *
LDUserNew(const char *key)
{
    LDUser *user;

    user = LDAlloc(sizeof(*user));
    if (!user) {
        LDi_log(2, "no memory for user\n");
        return NULL;
    }
    memset(user, 0, sizeof(*user));
    LDSetString(&user->key, key);
    user->anonymous = false;
    user->secondary = NULL;
    user->ip = NULL;
    user->firstName = NULL;
    user->lastName = NULL;
    user->email = NULL;
    user->name = NULL;
    user->avatar = NULL;
    user->custom = NULL;
    user->privateAttributeNames = NULL;

    return user;
}

static void
freeuser(LDUser *user)
{
    if (!user)
        return;
    LDFree(user->key);
    LDFree(user->secondary);
    LDFree(user->firstName);
    LDFree(user->lastName);
    LDFree(user->email);
    LDFree(user->name);
    LDFree(user->avatar);
    LDi_freehash(user->privateAttributeNames);
    LDFree(user);
}


static void
starteverything(void)
{

    curl_global_init(CURL_GLOBAL_DEFAULT);

    theClient = LDAlloc(sizeof(*theClient));
    if (!theClient) {
        LDi_log(2, "no memory for the client\n");
        return;
    }
    memset(theClient, 0, sizeof(*theClient));

    LDi_startthreads(theClient);
}


static void
checkconfig(LDConfig *config)
{
    if (config->pollingIntervalMillis < 300000)
        config->pollingIntervalMillis = 300000;

}

LDClient *
LDClientInit(LDConfig *config, LDUser *user)
{
    checkconfig(config);

    LDi_initevents(config->eventsCapacity);

    LDi_wrlock(&LDi_clientlock);

    pthread_once(&clientonce, starteverything);
    if (!theClient) {
        return NULL;
    }

    if (theClient->config != config) {
        freeconfig(theClient->config);
    }
    theClient->config = config;
    if (theClient->user != user) {
        freeuser(theClient->user);
    }
    theClient->user = user;
    theClient->dead = false;
    theClient->offline = config->offline;

    theClient->allFlags = NULL;
    theClient->isinit = false;

    LDi_unlock(&LDi_clientlock);

    LDi_recordidentify(user);

    LDi_log(10, "init done\n");
    return theClient;
}

LDClient *
LDClientGet()
{
    return theClient;
}

void
LDClientSetOffline(LDClient *client)
{
    LDi_wrlock(&LDi_clientlock);
    client->offline = true;
    LDi_unlock(&LDi_clientlock);
}

void
LDClientSetOnline(LDClient *client)
{
    LDi_wrlock(&LDi_clientlock);
    client->offline = false;
    client->isinit = false;
    LDi_unlock(&LDi_clientlock);
}

bool
LDClientIsOffline(LDClient *client)
{
    LDi_rdlock(&LDi_clientlock);
    bool offline = client->offline;
    LDi_unlock(&LDi_clientlock);
    return offline;
}

void
LDClientIdentify(LDClient *client, LDUser *user)
{
    LDi_wrlock(&LDi_clientlock);
    if (user != client->user) {
        freeuser(client->user);
    }
    client->user = user;
    LDi_unlock(&LDi_clientlock);
    LDi_recordidentify(user);
}

void
LDClientClose(LDClient *client)
{
    LDMapNode *oldhash;

    LDi_wrlock(&LDi_clientlock);
    client->dead = true;
    oldhash = client->allFlags;
    client->allFlags = NULL;
    freeconfig(client->config);
    client->config = NULL;
    freeuser(client->user);
    client->user = NULL;
    LDi_unlock(&LDi_clientlock);

    LDi_freehash(oldhash);

    /* stop the threads */
}

bool
LDClientIsInitialized(LDClient *client)
{
    LDi_rdlock(&LDi_clientlock);
    bool isinit = client->isinit;
    LDi_unlock(&LDi_clientlock);
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
LDClientSaveFlags(LDClient *client)
{
    LDi_rdlock(&LDi_clientlock);
    char *s = LDi_hashtostring(client->allFlags);
    LDi_unlock(&LDi_clientlock);
    return s;
}

void
LDClientRestoreFlags(LDClient *client, const char *data)
{
    LDi_clientsetflags(client, data, 0);
    if (LDi_statuscallback)
        LDi_statuscallback(1);
}

void
LDi_clientsetflags(LDClient *client, const char *data, int flavor)
{
    cJSON *payload = cJSON_Parse(data);

    if (!payload) {
        LDi_log(5, "parsing failed\n");
        return;
    }
    LDMapNode *hash = NULL;
    if (payload->type == cJSON_Object) {
        hash = LDi_jsontohash(payload, flavor);
    }
    cJSON_Delete(payload);

    LDi_wrlock(&LDi_clientlock);
    LDMapNode *oldhash = client->allFlags;
    client->allFlags = hash;
    client->isinit = true;
    LDi_unlock(&LDi_clientlock);

    LDi_freehash(oldhash);
}

/*
 * a block of functions to look up feature flags
 */

 bool
 isPrivateAttr(LDClient *client, const char *key)
 {
     return (LDMapLookup(client->config->privateAttributeNames, key) != NULL) ||
        (LDMapLookup(client->user->privateAttributeNames, key) != NULL);
 }

bool
LDBoolVariation(LDClient *client, const char *key, bool fallback)
{
    LDMapNode *res;
    bool b;

    LDi_rdlock(&LDi_clientlock);
    res = LDMapLookup(client->allFlags, key);
    if (res && res->type == LDNodeBool) {
        LDi_log(15, "found result for %s\n", key);
        b = res->b;
    } else {
        LDi_log(15, "no result for %s\n", key);
        b = fallback;
    }
    if (!isPrivateAttr(client, key))
        LDi_recordfeature(client->user, key, LDNodeBool,
            (double)b, NULL, NULL, (double)fallback, NULL, NULL);
    LDi_unlock(&LDi_clientlock);
    return b;
}

int
LDIntVariation(LDClient *client, const char *key, int fallback)
{
    LDMapNode *res;
    int i;

    LDi_rdlock(&LDi_clientlock);
    res = LDMapLookup(client->allFlags, key);
    if (res && res->type == LDNodeNumber)
        i = (int)res->n;
    else
        i = fallback;
    if (!isPrivateAttr(client, key))
        LDi_recordfeature(client->user, key, LDNodeNumber,
            (double)i, NULL, NULL, (double)fallback, NULL, NULL);
    LDi_unlock(&LDi_clientlock);
    return i;
}

double
LDDoubleVariation(LDClient *client, const char *key, double fallback)
{
    LDMapNode *res;
    double d;

    LDi_rdlock(&LDi_clientlock);
    res = LDMapLookup(client->allFlags, key);
    if (res && res->type == LDNodeNumber)
        d = res->n;
    else
        d = fallback;
    if (!isPrivateAttr(client, key))
        LDi_recordfeature(client->user, key, LDNodeNumber,
            d, NULL, NULL, fallback, NULL, NULL);
    LDi_unlock(&LDi_clientlock);
    return d;
}

char *
LDStringVariation(LDClient *client, const char *key, const char *fallback,
    char *buffer, size_t space)
{
    LDMapNode *res;
    const char *s;
    size_t len;

    LDi_rdlock(&LDi_clientlock);
    res = LDMapLookup(client->allFlags, key);
    if (res && res->type == LDNodeString)
        s = res->s;
    else
        s = fallback;
    
    len = strlen(s);
    if (len > space - 1)
        len = space - 1;
    memcpy(buffer, s, len);
    buffer[len] = 0;
    if (!isPrivateAttr(client, key))
        LDi_recordfeature(client->user, key, LDNodeString,
            0.0, buffer, NULL, 0.0, fallback, NULL);
    LDi_unlock(&LDi_clientlock);
    return buffer;
}

char *
LDStringVariationAlloc(LDClient *client, const char *key, const char *fallback)
{
    LDMapNode *res;
    const char *s;
    char *news;

    LDi_rdlock(&LDi_clientlock);
    res = LDMapLookup(client->allFlags, key);
    if (res && res->type == LDNodeString)
        s = res->s;
    else
        s = fallback;
    
    news = LDi_strdup(s);
    if (!isPrivateAttr(client, key))
        LDi_recordfeature(client->user, key, LDNodeString,
            0.0, news, NULL, 0.0, fallback, NULL);
    LDi_unlock(&LDi_clientlock);
    return news;
}

LDMapNode *
LDJSONVariation(LDClient *client, const char *key, LDMapNode *fallback)
{
    LDMapNode *res;
    LDMapNode *j;

    LDi_rdlock(&LDi_clientlock);
    res = LDMapLookup(client->allFlags, key);
    if (res && res->type == LDNodeMap) {
        j = res->m;
    } else {
        j = fallback;
    }
    if (!isPrivateAttr(client, key))
        LDi_recordfeature(client->user, key, LDNodeMap,
            0.0, NULL, j, 0.0, NULL, fallback);
    return j;
}

void
LDJSONRelease(LDMapNode *m)
{
    LDi_unlock(&LDi_clientlock);
}

void
LDClientFlush(LDClient *client)
{
    pthread_cond_signal(&LDi_bgeventcond);
}

bool
LDClientRegisterFeatureFlagListener(LDClient *client, const char *key, LDlistenerfn fn)
{
    struct listener *list;

    list = LDAlloc(sizeof(*list));
    if (!list)
        return false;
    list->fn = fn;
    list->key = LDi_strdup(key);
    if (!list->key) {
        LDFree(list);
        return false;
    }

    LDi_wrlock(&LDi_clientlock);
    list->next = client->listeners;
    client->listeners = list;
    LDi_unlock(&LDi_clientlock);

    return true;
}

bool
LDClientUnregisterFeatureFlagListener(LDClient *client, const char *key, LDlistenerfn fn)
{
    struct listener *list, *prev;

    prev = NULL;
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
    LDi_unlock(&LDi_clientlock);
    return list != NULL;
}

void
LDConfigAddPrivateAttribute(LDConfig *config, const char *key)
{
    LDMapNode *node = LDAlloc(sizeof(*node));
    memset(node, 0, sizeof(*node));
    node->key = LDi_strdup(key);
    node->type = LDNodeBool;
    node->b = true;

    HASH_ADD_KEYPTR(hh, config->privateAttributeNames, node->key, strlen(node->key), node);
}

void
LDUserAddPrivateAttribute(LDUser *user, const char *key)
{
    LDMapNode *node = LDAlloc(sizeof(*node));
    memset(node, 0, sizeof(*node));
    node->key = LDi_strdup(key);
    node->type = LDNodeBool;
    node->b = true;

    HASH_ADD_KEYPTR(hh, user->privateAttributeNames, node->key, strlen(node->key), node);
}
