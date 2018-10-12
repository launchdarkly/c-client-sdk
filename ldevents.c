#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ldapi.h"
#include "ldinternal.h"

static double
milliTimestamp(void)
{
    double t;

    t = time(NULL);
    t *= 1000.0;
    return t;
}

static cJSON *eventarray;
static int numevents;
static int eventscapacity;
static ld_rwlock_t eventlock = LD_RWLOCK_INIT;
static LDNode *summaryEvent;
static double summaryStart;

static void
initevents(int capacity)
{
    if (!eventarray)
        eventarray = cJSON_CreateArray();
    eventscapacity = capacity;
}

void
LDi_initevents(int capacity)
{
    LDi_wrlock(&eventlock);
    initevents(capacity);
    LDi_wrunlock(&eventlock);
}

void
LDi_recordidentify(LDUser *lduser)
{
    if (numevents >= eventscapacity) {
        return;
    }

    cJSON *json;

    json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "kind", "identify");
    cJSON_AddStringToObject(json, "key", lduser->key);
    cJSON_AddNumberToObject(json, "creationDate", milliTimestamp());
    cJSON *juser = LDi_usertojson(lduser);
    cJSON_AddItemToObject(json, "user", juser);

    LDi_wrlock(&eventlock);
    cJSON_AddItemToArray(eventarray, json);
    numevents++;
    LDi_wrunlock(&eventlock);
}

/*
 The summary event looks like this in memory...
 summaryEvent -> [feature] -> (hash) -> "default" -> (value)
                                     -> [countername] -> (value and count)
 */

static void
summarizeEvent(LDUser *lduser, LDNode *res, const char *feature, int type, double n, const char *s,
    LDNode *m, double defaultn, const char *defaults, LDNode *defaultm)
{
    LDNode *summary;

    LDi_log(40, "updating summary for %s\n", feature);

    LDi_wrlock(&eventlock);

    summary = LDNodeLookup(summaryEvent, feature);
    if (!summary) {
        summary = LDNodeCreateHash();
        switch (type) {
        case LDNodeString:
            LDNodeAddString(&summary, "default", defaults);
            break;
        case LDNodeNumber:
            LDNodeAddNumber(&summary, "default", defaultn);
            break;
        case LDNodeBool:
            LDNodeAddBool(&summary, "default", defaultn);
            break;
        case LDNodeHash:
            LDNodeAddHash(&summary, "default", defaultm);
            break;
        default:
            LDNodeAddString(&summary, "default", "");
            break;
        }
        summary = LDNodeAddHash(&summaryEvent, feature, summary);
    }
    if (summaryStart == 0) {
        summaryStart = milliTimestamp();
    }

    if (res) {
        char countername[128];
        snprintf(countername, sizeof(countername), "%d %d", res->version, res->variation);
        LDNode *counter;

        counter = LDNodeLookup(summary->h, countername);
        if (!counter) {
            switch (res->type) {
            case LDNodeString:
                counter = LDNodeAddString(&summary->h, countername, s);
                break;
            case LDNodeNumber:
                counter = LDNodeAddNumber(&summary->h, countername, n);
                break;
            case LDNodeBool:
                counter = LDNodeAddBool(&summary->h, countername, n);
                break;
            case LDNodeHash:
                counter = LDNodeAddHash(&summary->h, countername, m);
                break;
            default:
                counter = LDNodeAddString(&summary->h, countername, "");
                break;
            }
            counter->version = res->version;
            counter->variation = res->variation;
            counter->flagversion = res->flagversion;
        }
        counter->track++;
    }
    LDi_wrunlock(&eventlock);
}

static void
collectSummary()
{
    cJSON *json;

    if (summaryStart == 0) {
        return;
    }

    LDi_wrlock(&eventlock);

    json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "kind", "summary");
    cJSON_AddNumberToObject(json, "startDate", summaryStart);
    cJSON_AddNumberToObject(json, "endDate", milliTimestamp());
    cJSON *features = cJSON_CreateObject();
    LDNode *feature, *tmp;
    HASH_ITER(hh, summaryEvent, feature, tmp) {
        LDi_log(40, "json summary for %s\n", feature->key);
        cJSON *jfeature = cJSON_CreateObject();
        cJSON *counterarray = cJSON_CreateArray();

        LDNode *counter, *tmp2;
        HASH_ITER(hh, feature->h, counter, tmp2) {
            LDi_log(40, "json summary variation %s\n", counter->key);
            if (strcmp(counter->key, "default") == 0) {
                switch (counter->type) {
                case LDNodeNumber:
                    cJSON_AddNumberToObject(jfeature, "default", counter->n);
                    break;
                case LDNodeString:
                    cJSON_AddStringToObject(jfeature, "default", counter->s);
                    break;
                }
                continue;
            }
            cJSON *jcounter = cJSON_CreateObject();
            switch (counter->type) {
            case LDNodeNumber:
                cJSON_AddNumberToObject(jcounter, "value", counter->n);
                break;
            case LDNodeString:
                cJSON_AddStringToObject(jcounter, "value", counter->s);
                break;
            }
            cJSON_AddNumberToObject(jcounter, "version", counter->flagversion ? counter->flagversion : counter->version);
            cJSON_AddNumberToObject(jcounter, "count", counter->track);
            cJSON_AddNumberToObject(jcounter, "variation", counter->variation);

            cJSON_AddItemToArray(counterarray, jcounter);
        }
        cJSON_AddItemToObject(jfeature, "counters", counterarray);
        cJSON_AddItemToObject(features, feature->key, jfeature);

    }
    cJSON_AddItemToObject(json, "features", features);
    LDNodeFree(&summaryEvent);
    summaryStart = 0;

    cJSON_AddItemToArray(eventarray, json);
    numevents++;
    LDi_wrunlock(&eventlock);

}

void
LDi_recordfeature(LDUser *lduser, LDNode *res, const char *feature, int type, double n, const char *s,
    LDNode *m, double defaultn, const char *defaults, LDNode *defaultm)
{
    summarizeEvent(lduser, res, feature, type, n, s, m, defaultn, defaults, defaultm);

    if (!res || res->track == 0 || (res->track > 1 && res->track < milliTimestamp())) {
        return;
    }

    LDi_log(40, "choosing to track %s %d\n", feature, res->track);
    if (numevents >= eventscapacity) {
        LDi_log(5, "event capacity exceeded\n");
        return;
    }
    cJSON *json;

    json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "kind", "feature");
    cJSON_AddStringToObject(json, "key", feature);
    cJSON_AddNumberToObject(json, "creationDate", milliTimestamp());
    if (res) {
        cJSON_AddNumberToObject(json, "version", res->flagversion ? res->flagversion : res->version);
        cJSON_AddNumberToObject(json, "variation", res->variation);
    }
    if (type == LDNodeNumber) {
        cJSON_AddNumberToObject(json, "value", n);
        cJSON_AddNumberToObject(json, "default", defaultn);
    } else if (type == LDNodeString) {
        cJSON_AddStringToObject(json, "value", s);
        cJSON_AddStringToObject(json, "default", defaults);
    } else if (type == LDNodeBool) {
        cJSON_AddBoolToObject(json, "value", (int)n);
        cJSON_AddBoolToObject(json, "default", (int)defaultn);
    } else if (type == LDNodeHash) {
        cJSON_AddItemToObject(json, "value", LDi_hashtojson(m));
        cJSON_AddItemToObject(json, "default", LDi_hashtojson(defaultm));
    }
    cJSON *juser = LDi_usertojson(lduser);
    cJSON_AddItemToObject(json, "user", juser);

    LDi_wrlock(&eventlock);
    cJSON_AddItemToArray(eventarray, json);
    numevents++;
    LDi_wrunlock(&eventlock);
}

void
LDi_recordtrack(LDUser *user, const char *name, LDNode *data)
{
    if (numevents >= eventscapacity) {
        return;
    }
    cJSON *json;

    json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "kind", "custom");
    cJSON_AddStringToObject(json, "key", name);
    cJSON_AddNumberToObject(json, "creationDate", milliTimestamp());
    cJSON *juser = LDi_usertojson(user);
    cJSON_AddItemToObject(json, "user", juser);
    if (data != NULL) {
        cJSON_AddItemToObject(json, "data", LDi_hashtojson(data));
    }

    LDi_wrlock(&eventlock);
    cJSON_AddItemToArray(eventarray, json);
    numevents++;
    LDi_wrunlock(&eventlock);
}

char *
LDi_geteventdata(void)
{
    int hadevents;
    cJSON *events;

    collectSummary();

    LDi_wrlock(&eventlock);
    events = eventarray;
    eventarray = NULL;
    hadevents = numevents;
    numevents = 0;
    initevents(eventscapacity);
    LDi_wrunlock(&eventlock);

    char *data = NULL;
    if (hadevents) {
        data = cJSON_PrintUnformatted(events);
    }
    cJSON_Delete(events);
    return data;
}
