#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ldapi.h"
#include "ldinternal.h"

static double
milliTimestamp(void)
{
    return (double)time(NULL) * 1000.0;
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
    if (!eventarray) {
        eventarray = cJSON_CreateArray();
    }
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
LDi_recordidentify(LDClient *const client, LDUser *const lduser)
{
    LDi_wrlock(&eventlock);
    if (numevents >= eventscapacity) {
        LDi_wrunlock(&eventlock);
        LDi_log(LD_LOG_WARNING, "event capacity exceeded\n");
        return;
    }

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "kind", "identify");
    cJSON_AddStringToObject(json, "key", lduser->key);
    cJSON_AddNumberToObject(json, "creationDate", milliTimestamp());
    cJSON *const juser = LDi_usertojson(client, lduser, true);
    cJSON_AddItemToObject(json, "user", juser);

    cJSON_AddItemToArray(eventarray, json);
    numevents++;
    LDi_wrunlock(&eventlock);
}

/*
 The summary event looks like this in memory...
 summaryEvent -> [feature] -> (hash) -> "default" -> (value)
                                     -> [countername] -> (value and count)
 */

static LDNode*
addValueToHash(LDNode **hash, const char *name, int type, double n, const char *s, LDNode *m)
{
    switch (type) {
    case LDNodeString:
        return LDNodeAddString(hash, name, s);
        break;
    case LDNodeNumber:
        return LDNodeAddNumber(hash, name, n);
        break;
    case LDNodeBool:
        return LDNodeAddBool(hash, name, n);
        break;
    case LDNodeHash:
        return LDNodeAddHash(hash, name, m);
        break;
    default:
        return LDNodeAddString(hash, name, "");
        break;
    }
};

static void
addNodeToJSONObject(cJSON *obj, const char *key, LDNode *node)
{
    switch (node->type) {
    case LDNodeNumber:
        cJSON_AddNumberToObject(obj, key, node->n);
        break;
    case LDNodeString:
        cJSON_AddStringToObject(obj, key, node->s);
        break;
    case LDNodeBool:
        cJSON_AddBoolToObject(obj, key, (int)node->b);
        break;
    case LDNodeHash:
        cJSON_AddItemToObject(obj, key, LDi_hashtojson(node->h));
        break;
    }
};

static void
summarizeEvent(LDUser *lduser, LDNode *res, const char *feature, int type, double n, const char *s,
    LDNode *m, double defaultn, const char *defaults, LDNode *defaultm)
{
    LDi_log(LD_LOG_TRACE, "updating summary for %s\n", feature);

    LDi_wrlock(&eventlock);

    LDNode *summary = LDNodeLookup(summaryEvent, feature);

    if (!summary) {
        summary = LDNodeCreateHash();

        addValueToHash(&summary, "default", type, defaultn, defaults, defaultm);

        summary = LDNodeAddHash(&summaryEvent, feature, summary);
    }

    if (summaryStart == 0) {
        summaryStart = milliTimestamp();
    }

    char countername[128]; int keystatus;

    if (!res) {
        keystatus = snprintf(countername, sizeof(countername), "unknown");
    }
    else if (type == res->type) {
        const int version = res->flagversion ? res->flagversion : res->version;
        keystatus = snprintf(countername, sizeof(countername), "%d %d", version, res->variation);
    }
    else {
        keystatus = snprintf(countername, sizeof(countername), "default");
    }

    if (keystatus < 0) {
        LDi_log(LD_LOG_CRITICAL, "preparing key failed in summarizeEvent\n");
        LDi_wrunlock(&eventlock); return;
    }

    LDNode *counter = LDNodeLookup(summary->h, countername);

    if (!counter) {
        counter = addValueToHash(&summary->h, countername, type, n, s, m);
    }

    // may not have been set on initial recording based on res status
    if (res && counter->track == 0) {
        counter->version = res->version;
        counter->flagversion = res->flagversion;
        counter->variation = res->variation;
    }

    counter->track++;

    LDi_wrunlock(&eventlock);
}

static void
collectSummary()
{
    if (summaryStart == 0) { return; }

    LDi_wrlock(&eventlock);

    cJSON *const json = cJSON_CreateObject();
    cJSON *const features = cJSON_CreateObject();

    cJSON_AddNumberToObject(json, "startDate", summaryStart);
    cJSON_AddNumberToObject(json, "endDate", milliTimestamp());

    LDNode *feature, *tmp;
    HASH_ITER(hh, summaryEvent, feature, tmp) {
        cJSON *const jfeature = cJSON_CreateObject();
        cJSON *const counterarray = cJSON_CreateArray();

        LDNode *counter, *tmp2;
        HASH_ITER(hh, feature->h, counter, tmp2) {
            if (strcmp(counter->key, "default") == 0) {
                addNodeToJSONObject(jfeature, "default", counter);
                // don't record default in flags if not used
                if (counter->track == 0) { continue; }
            }

            cJSON *const jcounter = cJSON_CreateObject();
            addNodeToJSONObject(jcounter, "value", counter);

            if (strcmp(counter->key, "unknown") == 0) {
                cJSON_AddBoolToObject(jcounter, "unknown", true);
            }
            else {
                cJSON_AddNumberToObject(jcounter, "version", counter->flagversion ? counter->flagversion : counter->version);

                if (strcmp(counter->key, "default") != 0) {
                    cJSON_AddNumberToObject(jcounter, "variation", counter->variation);
                }
            }

            cJSON_AddNumberToObject(jcounter, "count", counter->track);

            cJSON_AddItemToArray(counterarray, jcounter);
        }
        cJSON_AddItemToObject(jfeature, "counters", counterarray);
        cJSON_AddItemToObject(features, feature->key, jfeature);
    }

    cJSON_AddItemToObject(json, "features", features);
    cJSON_AddStringToObject(json, "kind", "summary");
    LDNodeFree(&summaryEvent);
    summaryStart = 0;

    cJSON_AddItemToArray(eventarray, json);
    numevents++;
    LDi_wrunlock(&eventlock);
}

void
LDi_recordfeature(LDClient *client, LDUser *lduser, LDNode *res, const char *feature,
    int type, double n, const char *s, LDNode *m, double defaultn, const char *defaults, LDNode *defaultm)
{
    summarizeEvent(lduser, res, feature, type, n, s, m, defaultn, defaults, defaultm);

    if (!res || res->track == 0 || (res->track > 1 && res->track < milliTimestamp())) {
        return;
    }

    LDi_wrlock(&eventlock);
    if (numevents >= eventscapacity) {
        LDi_wrunlock(&eventlock);
        LDi_log(LD_LOG_WARNING, "LDi_recordfeature event capacity exceeded\n");
        return;
    }

    cJSON *const json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "kind", "feature");
    cJSON_AddStringToObject(json, "key", feature);
    cJSON_AddNumberToObject(json, "creationDate", milliTimestamp());

    cJSON_AddNumberToObject(json, "version", res->flagversion ? res->flagversion : res->version);

    if (type == res->type) {
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

    cJSON *const juser = LDi_usertojson(client, lduser, true);
    cJSON_AddItemToObject(json, "user", juser);

    cJSON_AddItemToArray(eventarray, json);
    numevents++;
    LDi_wrunlock(&eventlock);
}

void
LDi_recordtrack(LDClient *client, LDUser *user, const char *name, LDNode *data)
{
    LDi_wrlock(&eventlock);
    if (numevents >= eventscapacity) {
        LDi_wrunlock(&eventlock);
        LDi_log(LD_LOG_WARNING, "event capacity exceeded\n");
        return;
    }

    cJSON *const json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "kind", "custom");
    cJSON_AddStringToObject(json, "key", name);
    cJSON_AddNumberToObject(json, "creationDate", milliTimestamp());

    cJSON *const juser = LDi_usertojson(client, user, true);
    cJSON_AddItemToObject(json, "user", juser);

    if (data != NULL) {
        cJSON_AddItemToObject(json, "data", LDi_hashtojson(data));
    }

    cJSON_AddItemToArray(eventarray, json);
    numevents++;
    LDi_wrunlock(&eventlock);
}

char *
LDi_geteventdata(void)
{
    collectSummary();

    LDi_wrlock(&eventlock);
    cJSON *const events = eventarray;
    eventarray = NULL;
    const int hadevents = numevents;
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
