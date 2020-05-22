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

static void
enqueueEvent(LDClient *const client, cJSON *const event)
{
    if (client->numEvents >= client->shared->sharedConfig->eventsCapacity) {
        LDi_log(LD_LOG_WARNING, "event capacity exceeded");
        cJSON_Delete(event);
        return;
    }

    cJSON_AddItemToArray(client->eventArray, event);
    client->numEvents++;
}

static cJSON *
makeBaseEvent(LDClient *const client, LDUser *const lduser,
    const char *const kind)
{
    cJSON *event, *jsonuser;

    LD_ASSERT(event = cJSON_CreateObject());

    cJSON_AddStringToObject(event, "kind", kind);
    cJSON_AddNumberToObject(event, "creationDate", milliTimestamp());
    LD_ASSERT(jsonuser = LDi_usertojson(client, lduser, true));
    cJSON_AddItemToObject(event, "user", jsonuser);

    return event;
}

static cJSON *
makeTrackEvent(LDClient *const client, LDUser *const lduser,
    const char *const name, LDNode *const data)
{
    cJSON *const event = makeBaseEvent(client, lduser, "custom");
    cJSON_AddStringToObject(event, "key", name);

    if (data != NULL) {
        cJSON_AddItemToObject(event, "data", LDi_hashtojson(data));
    }

    return event;
}

static cJSON *
makeTrackMetricEvent(LDClient *const client, LDUser *const lduser,
    const char *const name, LDNode *const data, const double metric)
{
    cJSON *const event = makeTrackEvent(client, lduser, name, data);

    cJSON_AddNumberToObject(event, "metricValue", metric);

    return event;
}

void
LDi_recordidentify(LDClient *const client, LDUser *const lduser)
{
    cJSON *json = makeBaseEvent(client, lduser, "identify");
    cJSON_AddStringToObject(json, "key", lduser->key);

    LDi_wrlock(&client->eventLock);

    enqueueEvent(client, json);

    LDi_wrunlock(&client->eventLock);
}

/*
 The summary event looks like this in memory...
 summaryEvent -> [feature] -> (hash) -> "default" -> (value)
                                     -> [countername] -> (value and count)
 */

static LDNode*
addValueToHash(LDNode **hash, const char *const name, const int type,
    const double n, const char *const s, const LDNode *const m)
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
        return LDNodeAddHash(hash, name, LDCloneHash(m));
        break;
    default:
        LDi_log(LD_LOG_FATAL, "addValueToHash unhandled case");
        abort();
        break;
    }
};

static void
addNodeToJSONObject(cJSON *const obj, const char *const key, LDNode *const node)
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
    case LDNodeArray:
        cJSON_AddItemToObject(obj, key, LDi_arraytojson(node->a));
        break;
    default:
        LDi_log(LD_LOG_FATAL, "addNodeToJSONObject unhandled case");
        abort();
        break;
    }
};

static void
summarizeEvent(LDClient *const client, LDUser *lduser, LDNode *res, const char *feature,
    const LDNodeType type, const double n, const char *const s, LDNode *const m,
    const double defaultn, const char *const defaults, const LDNode *const defaultm)
{
    LDi_log(LD_LOG_TRACE, "updating summary for %s", feature);

    LDi_wrlock(&client->eventLock);

    LDNode *summary = LDNodeLookup(client->summaryEvent, feature);

    if (!summary) {
        summary = LDNodeCreateHash();

        addValueToHash(&summary, "default", type, defaultn, defaults, defaultm);

        summary = LDNodeAddHash(&client->summaryEvent, feature, summary);
    }

    if (client->summaryStart == 0) {
        client->summaryStart = milliTimestamp();
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
        LDi_log(LD_LOG_CRITICAL, "preparing key failed in summarizeEvent");
        LDi_wrunlock(&client->eventLock); return;
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

    LDi_wrunlock(&client->eventLock);
}

static void
collectSummary(LDClient *const client)
{
    LDi_wrlock(&client->eventLock);

    if (client->summaryStart == 0) {
        LDi_wrunlock(&client->eventLock);
        return;
    }

    cJSON *const json = cJSON_CreateObject();
    cJSON *const features = cJSON_CreateObject();

    cJSON_AddNumberToObject(json, "startDate", client->summaryStart);
    cJSON_AddNumberToObject(json, "endDate", milliTimestamp());

    LDNode *feature, *tmp;
    HASH_ITER(hh, client->summaryEvent, feature, tmp) {
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
    LDNodeFree(&client->summaryEvent);
    client->summaryStart = 0;

    enqueueEvent(client, json);

    LDi_wrunlock(&client->eventLock);
}

void
LDi_recordfeature(LDClient *const client, LDUser *const lduser, LDNode *const res,
    const char *const feature, const LDNodeType type, const double n, const char *const s, LDNode *const m,
    const double defaultn, const char *const defaults, const LDNode *const defaultm, const bool detail)
{
    summarizeEvent(client, lduser, res, feature, type, n, s, m, defaultn, defaults, defaultm);

    if (!res || res->track == 0 || (res->track > 1 && res->track < milliTimestamp())) {
        return;
    }

    cJSON *const json = makeBaseEvent(client, lduser, "feature");
    cJSON_AddStringToObject(json, "key", feature);

    cJSON_AddNumberToObject(json, "version", res->flagversion ? res->flagversion : res->version);

    if (type == res->type) {
        cJSON_AddNumberToObject(json, "variation", res->variation);
    }

    if (detail && res->reason) {
        cJSON_AddItemToObject(json, "reason", LDi_hashtojson(res->reason));
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

    LDi_wrlock(&client->eventLock);
    enqueueEvent(client, json);
    LDi_wrunlock(&client->eventLock);
}

void
LDi_recordtrack(LDClient *const client, LDUser *const user,
    const char *const name, LDNode *const data)
{
    cJSON *const event = makeTrackEvent(client, user, name, data);

    LDi_wrlock(&client->eventLock);
    enqueueEvent(client, event);
    LDi_wrunlock(&client->eventLock);
}

void
LDi_recordtrackmetric(LDClient *const client, LDUser *const user,
    const char *const name, LDNode *const data, const double metric)
{
    cJSON *const event = makeTrackMetricEvent(client, user, name, data, metric);

    LDi_wrlock(&client->eventLock);
    enqueueEvent(client, event);
    LDi_wrunlock(&client->eventLock);
}

char *
LDi_geteventdata(LDClient *const client)
{
    collectSummary(client);

    LDi_wrlock(&client->eventLock);

    if (client->numEvents == 0) {
        LDi_wrunlock(&client->eventLock);
        return NULL;
    }

    cJSON *const events = client->eventArray;
    client->eventArray = cJSON_CreateArray();
    client->numEvents = 0;

    LDi_wrunlock(&client->eventLock);

    char *const data = cJSON_PrintUnformatted(events);
    cJSON_Delete(events);
    return data;
}
