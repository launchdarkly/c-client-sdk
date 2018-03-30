#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include "curl/curl.h"

#include "ldapi.h"
#include "ldinternal.h"


static pthread_once_t clientonce = PTHREAD_ONCE_INIT;
static pthread_t eventthread;
static pthread_t pollingthread;

static LDClient *theClient;
static pthread_rwlock_t clientlock = PTHREAD_RWLOCK_INITIALIZER;

void
LDSetString(char **target, const char *value)
{
    free(*target);
    if (value) {
        *target = strdup(value);
    } else {
        *target = NULL;
    }
}

LDConfig *
LDConfigNew(const char *mobileKey)
{
    LDConfig *config;

    config = malloc(sizeof(*config));
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

LDUser *
LDUserNew(const char *key)
{
    LDUser *user;

    user = malloc(sizeof(*user));
    LDSetString(&user->key, key);
    user->anonymous = false;
    user->secondary = NULL;
    user->ip = NULL;
    user->firstName = NULL;
    user->lastName = NULL;
    user->avatar = NULL;
    user->custom = NULL;
    user->privateAttributeNames = NULL;

    return user;
}

static void
milliSleep(int ms)
{
    sleep(ms / 1000);
}

static void *
bgeventsender(void *v)
{
    LDClient *client = v;

    while (true) {
        LDi_rdlock(&clientlock);
        int ms = client->config->eventsFlushIntervalMillis;
        LDi_unlock(&clientlock);

        

        printf("bg sender sleeping\n");
        milliSleep(ms);
        printf("bgsender running\n");

        char *eventdata = LDi_geteventdata();
        if (!eventdata) {
            printf("no event data to send\n");
            continue;
        }

        LDi_rdlock(&clientlock);
        if (client->dead) {
            LDi_unlock(&clientlock);
            continue;
        }
        
        bool sent = false;
        int retries = 0;
        while (!sent) {
            char url[4096];
            snprintf(url, sizeof(url), "%s/mobile", client->config->eventsURI);
            char authkey[256];
            snprintf(authkey, sizeof(authkey), "%s", client->config->mobileKey);
            LDi_unlock(&clientlock);
            /* unlocked while sending; will relock if retry needed */
            int response = 0;
            LDi_sendevents(url, authkey, eventdata, &response);
            if (response == 401 || response == 403) {
                sent = true; /* consider it done */
                client->dead = true;
                retries = 0;
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
                milliSleep(backoff);
                LDi_rdlock(&clientlock);
            }
        }
        free(eventdata);
    }
}

static void *
bgfeaturepoller(void *v)
{
    LDClient *client = v;
    char *prevdata = NULL;

    while (true) {
        LDi_rdlock(&clientlock);
        int ms = client->config->pollingIntervalMillis;
        LDi_unlock(&clientlock);

        printf("bg poller sleeping\n");
        milliSleep(ms);
        printf("bg poller running\n");

        LDi_rdlock(&clientlock);
        if (client->dead) {
            LDi_unlock(&clientlock);
            continue;
        }
        int response = 0;
        LDMapNode *hash = LDi_fetchfeaturemap(client, &response);
        if (response == 401 || response == 403) {
            client->dead = true;
        }
        LDi_unlock(&clientlock);
        if (!hash)
            continue;
        LDMapNode *oldhash;

        LDi_wrlock(&clientlock);
        oldhash = client->allFlags;
        client->allFlags = hash;
        LDi_unlock(&clientlock);
        LDi_freehash(oldhash);
    }
}

static void
starteverything(void)
{

    curl_global_init(CURL_GLOBAL_DEFAULT);

    theClient = malloc(sizeof(*theClient));
    memset(theClient, 0, sizeof(*theClient));

    pthread_create(&eventthread, NULL, bgeventsender, theClient);
    pthread_create(&pollingthread, NULL, bgfeaturepoller, theClient);
}

static void
checkconfig(LDConfig *config)
{


}

LDClient *
LDClientInit(LDConfig *config, LDUser *user)
{
    checkconfig(config);

    LDi_initevents(config->eventsCapacity);

    LDi_wrlock(&clientlock);

    pthread_once(&clientonce, starteverything);

    theClient->config = config;
    theClient->user = user;
    theClient->dead = false;

    int response;
    LDMapNode *hash = LDi_fetchfeaturemap(theClient, &response);
    theClient->allFlags = hash;

    LDi_unlock(&clientlock);

    LDi_recordidentify(user);

    printf("init done\n");
    return theClient;
}

LDClient *
LDClientGet()
{
    return theClient;
}

static LDMapNode *
lookupnode(LDMapNode *hash, const char *key)
{
    LDMapNode *res = NULL;

    HASH_FIND_STR(hash, key, res);
    
    return res;
}

bool
LDBoolVariation(LDClient *client, const char *key, bool fallback)
{
    LDMapNode *res;
    bool b;

    LDi_rdlock(&clientlock);
    res = lookupnode(client->allFlags, key);
    if (res && res->type == LDNodeBool)
        b = res->b;
    else
        b = fallback;
    LDi_unlock(&clientlock);
    LDi_recordfeature(client->user, key, LDNodeBool, (double)b, NULL, (double)fallback, NULL);
    return b;
}

int
LDIntVariation(LDClient *client, const char *key, int fallback)
{
    LDMapNode *res;
    int i;

    LDi_rdlock(&clientlock);
    res = lookupnode(client->allFlags, key);
    if (res && res->type == LDNodeNumber)
        i = (int)res->n;
    else
        i = fallback;
    LDi_unlock(&clientlock);
    LDi_recordfeature(client->user, key, LDNodeNumber, (double)i, NULL, (double)fallback, NULL);
    return i;
}

double
LDDoubleVariation(LDClient *client, const char *key, double fallback)
{
    LDMapNode *res;
    double d;

    LDi_rdlock(&clientlock);
    res = lookupnode(client->allFlags, key);
    if (res && res->type == LDNodeNumber)
        d = res->n;
    else
        d = fallback;
    LDi_unlock(&clientlock);
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

    LDi_rdlock(&clientlock);
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
    LDi_unlock(&clientlock);
    LDi_recordfeature(client->user, key, LDNodeString, 0.0, buffer, 0.0, fallback);
    return buffer;
}

char *
LDStringVariationAlloc(LDClient *client, const char *key, const char *fallback)
{
    LDMapNode *res;
    const char *s;
    char *news;

    LDi_rdlock(&clientlock);
    res = lookupnode(client->allFlags, key);
    if (res && res->type == LDNodeString)
        s = res->s;
    else
        s = fallback;
    
    news = strdup(s);
    LDi_unlock(&clientlock);
    LDi_recordfeature(client->user, key, LDNodeString, 0.0, news, 0.0, fallback);
    return news;
}