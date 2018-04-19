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

LDConfig *
LDConfigNew(const char *mobileKey)
{
    LDConfig *config;

    config = malloc(sizeof(*config));
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
    free(config->appURI);
    free(config->eventsURI);
    free(config->mobileKey);
    free(config->streamURI);
    /* free privateAttributeNames */
    free(config);
}


LDUser *
LDUserNew(const char *key)
{
    LDUser *user;

    user = malloc(sizeof(*user));
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
    free(user->key);
    free(user->secondary);
    free(user->firstName);
    free(user->lastName);
    free(user->email);
    free(user->name);
    free(user->avatar);
    /* free custom and privateAttributeNames */
    free(user);
}


static void
starteverything(void)
{

    curl_global_init(CURL_GLOBAL_DEFAULT);

    theClient = malloc(sizeof(*theClient));
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

    if (theClient->config != config) {
        freeconfig(theClient->config);
    }
    theClient->config = config;
    if (theClient->user != user) {
        freeuser(theClient->user);
    }
    theClient->user = user;
    theClient->dead = false;

    LDMapNode *hash = NULL;
    theClient->allFlags = hash;

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

/*
 * a block of functions to look up feature flags
 */
static LDMapNode *
lookupnode(LDMapNode *hash, const char *key)
{
    LDMapNode *res = NULL;

    LDMapNode *node, *tmp;

    HASH_FIND_STR(hash, key, res);   
    return res;
}

bool
LDBoolVariation(LDClient *client, const char *key, bool fallback)
{
    LDMapNode *res;
    bool b;

    LDi_rdlock(&LDi_clientlock);
    res = lookupnode(client->allFlags, key);
    if (res && res->type == LDNodeBool) {
        LDi_log(15, "found result\n");    
        b = res->b;
    } else {
        LDi_log(15, "no result for %s\n", key);
        b = fallback;
    }
    LDi_unlock(&LDi_clientlock);
    LDi_recordfeature(client->user, key, LDNodeBool, (double)b, NULL, (double)fallback, NULL);
    return b;
}

int
LDIntVariation(LDClient *client, const char *key, int fallback)
{
    LDMapNode *res;
    int i;

    LDi_rdlock(&LDi_clientlock);
    res = lookupnode(client->allFlags, key);
    if (res && res->type == LDNodeNumber)
        i = (int)res->n;
    else
        i = fallback;
    LDi_unlock(&LDi_clientlock);
    LDi_recordfeature(client->user, key, LDNodeNumber, (double)i, NULL, (double)fallback, NULL);
    return i;
}

double
LDDoubleVariation(LDClient *client, const char *key, double fallback)
{
    LDMapNode *res;
    double d;

    LDi_rdlock(&LDi_clientlock);
    res = lookupnode(client->allFlags, key);
    if (res && res->type == LDNodeNumber)
        d = res->n;
    else
        d = fallback;
    LDi_unlock(&LDi_clientlock);
    LDi_recordfeature(client->user, key, LDNodeNumber, d, NULL, fallback, NULL);
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
    res = lookupnode(client->allFlags, key);
    if (res && res->type == LDNodeString)
        s = res->s;
    else
        s = fallback;
    
    len = strlen(s);
    if (len > space - 1)
        len = space - 1;
    memcpy(buffer, s, len);
    buffer[len] = 0;
    LDi_unlock(&LDi_clientlock);
    LDi_recordfeature(client->user, key, LDNodeString, 0.0, buffer, 0.0, fallback);
    return buffer;
}

char *
LDStringVariationAlloc(LDClient *client, const char *key, const char *fallback)
{
    LDMapNode *res;
    const char *s;
    char *news;

    LDi_rdlock(&LDi_clientlock);
    res = lookupnode(client->allFlags, key);
    if (res && res->type == LDNodeString)
        s = res->s;
    else
        s = fallback;
    
    news = strdup(s);
    LDi_unlock(&LDi_clientlock);
    LDi_recordfeature(client->user, key, LDNodeString, 0.0, news, 0.0, fallback);
    return news;
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


    list = malloc(sizeof(*list));
    if (!list)
        return false;
    list->fn = fn;
    list->key = strdup(key);
    if (!list->key) {
        free(list);
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
            free(list->key);
            free(list);
            break;
        }
    }
    LDi_unlock(&LDi_clientlock);
    return list != NULL;
}
