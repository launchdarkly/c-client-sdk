#include <string.h>
#include <stdio.h>

#include "event_processor.h"
#include "event_processor_internal.h"
#include "utility.h"
#include "ldinternal.h"

static int
LDi_getFlagVersion(const struct LDFlag *const flag)
{
    if (flag->flagVersion != -1) {
        return flag->flagVersion;
    }

    return flag->version;
}

struct EventProcessor *
LDi_newEventProcessor(const struct LDConfig *const config)
{
    struct EventProcessor *context;

    if (!(context =
        (struct EventProcessor *)LDAlloc(sizeof(struct EventProcessor))))
    {
        goto error;
    }

    context->summaryStart     = 0;
    context->lastUserKeyFlush = 0;
    context->lastServerTime   = 0;
    context->config           = config;

    LDi_getMonotonicMilliseconds(&context->lastUserKeyFlush);
    LDi_mutex_init(&context->lock);

    if (!(context->events = LDNewArray())) {
        goto error;
    }

    if (!(context->summaryCounters = LDNewObject())) {
        goto error;
    }

    return context;

  error:
    LDi_freeEventProcessor(context);

    return NULL;
}

void
LDi_freeEventProcessor(struct EventProcessor *const context)
{
    if (context) {
        LDi_mutex_destroy(&context->lock);
        LDJSONFree(context->events);
        LDJSONFree(context->summaryCounters);
        LDFree(context);
    }
}

void
LDi_addEvent(struct EventProcessor *const context, struct LDJSON *const event)
{
    LD_ASSERT(context);
    LD_ASSERT(event);

    /* sanity check */
    LD_ASSERT(LDJSONGetType(context->events) == LDArray);

    if (
        LDCollectionGetSize(context->events) >= context->config->eventsCapacity
    ) {
        LD_LOG(LD_LOG_WARNING, "event capacity exceeded, dropping event");
    } else {
        LDArrayPush(context->events, event);
    }
}

struct LDJSON *
LDi_newBaseEvent(const char *const kind, const double now)
{
    struct LDJSON *tmp, *event;

    tmp          = NULL;
    event        = NULL;

    if (!(event = LDNewObject())) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        goto error;
    }

    if (!(tmp = LDNewNumber(now))) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        goto error;
    }

    if (!(LDObjectSetKey(event, "creationDate", tmp))) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        LDJSONFree(tmp);

        goto error;
    }

    if (!(tmp = LDNewText(kind))) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        goto error;
    }

    if (!(LDObjectSetKey(event, "kind", tmp))) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        goto error;
    }

    return event;

  error:
    LDJSONFree(event);

    return NULL;
}

bool
LDi_addUserInfoToEvent(
    const struct EventProcessor *const context,
    struct LDJSON *const               event,
    const struct LDUser *const         user
) {
    struct LDJSON *tmp;

    LD_ASSERT(context);
    LD_ASSERT(event);
    LD_ASSERT(user);

    if (context->config->inlineUsersInEvents) {
        if (!(tmp = LDi_userToJSON(user, LDBooleanTrue,
            context->config->allAttributesPrivate,
            context->config->privateAttributeNames)))
        {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            return false;
        }

        if (!(LDObjectSetKey(event, "user", tmp))) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            LDJSONFree(tmp);

            return false;
        }
    } else {
        if (!(tmp = LDNewText(user->key))) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            return false;
        }

        if (!(LDObjectSetKey(event, "userKey", tmp))) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            LDJSONFree(tmp);

            return false;
        }
    }

    return true;
}

struct LDJSON *
LDi_newIdentifyEvent(
    const struct EventProcessor *const context,
    const struct LDUser *const         user,
    const double                       now
) {
    struct LDJSON *event, *tmp;

    LD_ASSERT(context);
    LD_ASSERT(user);

    event = NULL;
    tmp   = NULL;

    if (!(event = LDi_newBaseEvent("identify", now))) {
        LD_LOG(LD_LOG_ERROR, "failed to construct base event");

        return NULL;
    }

    if (!(tmp = LDNewText(user->key))) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        LDJSONFree(event);

        return false;
    }

    if (!(LDObjectSetKey(event, "key", tmp))) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        LDJSONFree(event);
        LDJSONFree(tmp);

        return false;
    }

    if (!(tmp = LDi_userToJSON(user, LDBooleanTrue,
        context->config->allAttributesPrivate,
        context->config->privateAttributeNames)))
    {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        LDJSONFree(event);

        return NULL;
    }

    if (!(LDObjectSetKey(event, "user", tmp))) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        LDJSONFree(tmp);
        LDJSONFree(event);

        return NULL;
    }

    return event;
}

bool
LDi_identify(
    struct EventProcessor *const context,
    const struct LDUser *const   user
) {
    struct LDJSON *event;
    double now;

    LD_ASSERT(context);
    LD_ASSERT(user);

    LDi_getUnixMilliseconds(&now);

    if (!(event = LDi_newIdentifyEvent(context, user, now))) {
        LD_LOG(LD_LOG_ERROR, "failed to construct identify event");

        return false;
    }

    LDi_mutex_lock(&context->lock);

    LDi_addEvent(context, event);

    LDi_mutex_unlock(&context->lock);

    return true;
}

struct LDJSON *
LDi_newCustomEvent(
    const struct EventProcessor *const context,
    const struct LDUser *const         user,
    const char *const                  key,
    struct LDJSON *const               data,
    const double                       metric,
    const bool                         hasMetric,
    const double                       now
) {
    struct LDJSON *tmp, *event;

    LD_ASSERT(context);
    LD_ASSERT(user);
    LD_ASSERT(key);

    tmp   = NULL;
    event = NULL;

    if (!(event = LDi_newBaseEvent("custom", now))) {
        LD_LOG(LD_LOG_ERROR, "memory error");

        goto error;
    }

    if (!LDi_addUserInfoToEvent(context, event, user)) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        goto error;
    }

    if (!(tmp = LDNewText(key))) {
        LD_LOG(LD_LOG_ERROR, "memory error");

        goto error;
    }

    if (!LDObjectSetKey(event, "key", tmp)) {
        LD_LOG(LD_LOG_ERROR, "memory error");

        LDJSONFree(tmp);

        goto error;
    }

    if (data) {
        if (!LDObjectSetKey(event, "data", data)) {
            LD_LOG(LD_LOG_ERROR, "memory error");

            goto error;
        }
    }

    if (hasMetric) {
        if (!(tmp = LDNewNumber(metric))) {
            LD_LOG(LD_LOG_ERROR, "memory error");

            goto error;
        }

        if (!LDObjectSetKey(event, "metricValue", tmp)) {
            LD_LOG(LD_LOG_ERROR, "memory error");

            LDJSONFree(tmp);

            goto error;
        }
    }

    if (user->anonymous) {
        if (!(tmp = LDNewText("anonymousUser"))) {
            goto error;
        }

        if (!LDObjectSetKey(event, "contextKind", tmp)) {
            LDJSONFree(tmp);

            goto error;
        }
    }

    return event;

  error:
    LDJSONFree(event);

    return NULL;
}

static struct LDJSON *
contextKindString(const struct LDUser *const user)
{
    if (user->anonymous) {
        return LDNewText("anonymousUser");
    } else {
        return LDNewText("user");
    }
}

struct LDJSON *
LDi_newAliasEvent(
    const struct LDUser *const   currentUser,
    const struct LDUser *const   previousUser,
    const double                 now
) {
    struct LDJSON *tmp, *event;

    LD_ASSERT(currentUser);
    LD_ASSERT(previousUser);

    tmp   = NULL;
    event = NULL;

    if (!(event = LDi_newBaseEvent("alias", now))) {
        goto error;
    }

    if (!(tmp = LDNewText(currentUser->key))) {
        goto error;
    }

    if (!LDObjectSetKey(event, "key", tmp)) {
        goto error;
    }

    if (!(tmp = LDNewText(previousUser->key))) {
        goto error;
    }

    if (!LDObjectSetKey(event, "previousKey", tmp)) {
        goto error;
    }

    if (!(tmp = contextKindString(currentUser))) {
        goto error;
    }

    if (!LDObjectSetKey(event, "contextKind", tmp)) {
        goto error;
    }

    if (!(tmp = contextKindString(previousUser))) {
        goto error;
    }

    if (!LDObjectSetKey(event, "previousContextKind", tmp)) {
        goto error;
    }

    return event;

  error:
    LDJSONFree(event);
    LDJSONFree(tmp);

    return NULL;
}

bool
LDi_track(
    struct EventProcessor *const context,
    const struct LDUser *const   user,
    const char *const            key,
    struct LDJSON *const         data,
    const double                 metric,
    const bool                   hasMetric
) {
    struct LDJSON *event;
    double now;

    LD_ASSERT(context);
    LD_ASSERT(user);

    LDi_getUnixMilliseconds(&now);

    LDi_mutex_lock(&context->lock);

    if (!(event = LDi_newCustomEvent(
        context, user, key, data, metric, hasMetric, now)))
    {
        LD_LOG(LD_LOG_ERROR, "failed to construct custom event");

        LDi_mutex_unlock(&context->lock);

        return false;
    }

    LDi_addEvent(context, event);

    LDi_mutex_unlock(&context->lock);

    return true;
}

bool
LDi_alias(
    struct EventProcessor *const context,
    const struct LDUser *const   currentUser,
    const struct LDUser *const   previousUser
) {
    struct LDJSON *event;
    double now;

    LD_ASSERT(context);
    LD_ASSERT(currentUser);
    LD_ASSERT(previousUser);

    LDi_getUnixMilliseconds(&now);

    LDi_mutex_lock(&context->lock);

    if (!(event = LDi_newAliasEvent(currentUser, previousUser, now))) {
        LD_LOG(LD_LOG_ERROR, "failed to construct alias event");

        LDi_mutex_unlock(&context->lock);

        return false;
    }

    LDi_addEvent(context, event);

    LDi_mutex_unlock(&context->lock);

    return true;
}

struct LDJSON *
LDi_objectToArray(const struct LDJSON *const object)
{
    struct LDJSON *iter, *array;

    LD_ASSERT(object);
    LD_ASSERT(LDJSONGetType(object) == LDObject);

    iter  = NULL;
    array = NULL;

    if (!(array = LDNewArray())) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        return NULL;
    }

    for (iter = LDGetIter(object); iter; iter = LDIterNext(iter)) {
        struct LDJSON *dupe;

        if (!(dupe = LDJSONDuplicate(iter))) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            LDJSONFree(array);

            return NULL;
        }

        LDArrayPush(array, dupe);
    }

    return array;
}

struct LDJSON *
LDi_prepareSummaryEvent(
    struct EventProcessor *const context,
    const double                 now
) {
    struct LDJSON *tmp, *summary, *iter, *counters;

    LD_ASSERT(context);

    tmp      = NULL;
    summary  = NULL;
    iter     = NULL;
    counters = NULL;

    if (!(summary = LDNewObject())) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        goto error;
    }

    if (!(tmp = LDNewText("summary"))) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        goto error;
    }

    if (!(LDObjectSetKey(summary, "kind", tmp))) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        LDJSONFree(tmp);

        goto error;
    }

    if (!(tmp = LDNewNumber(context->summaryStart))) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        goto error;
    }

    if (!(LDObjectSetKey(summary, "startDate", tmp))) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        LDJSONFree(tmp);

        goto error;
    }

    if (!(tmp = LDNewNumber(now))) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        goto error;
    }

    if (!(LDObjectSetKey(summary, "endDate", tmp))) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        LDJSONFree(tmp);

        goto error;
    }

    if (!(counters = LDJSONDuplicate(context->summaryCounters))) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        goto error;
    }

    for (iter = LDGetIter(counters); iter; iter = LDIterNext(iter)) {
        struct LDJSON *countersObject, *countersArray;

        countersObject = LDObjectDetachKey(iter, "counters");
        LD_ASSERT(countersObject);

        if (!(countersArray = LDi_objectToArray(countersObject))) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            LDJSONFree(countersObject);

            goto error;
        }

        LDJSONFree(countersObject);

        if (!LDObjectSetKey(iter, "counters", countersArray)) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            LDJSONFree(countersArray);

            goto error;
        }
    }

    if (!LDObjectSetKey(summary, "features", counters)) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        goto error;
    }

    return summary;

  error:
    LDJSONFree(summary);
    LDJSONFree(counters);

    return NULL;
}

bool
LDi_bundleEventPayload(
    struct EventProcessor *const context,
    struct LDJSON **const        result
) {
    struct LDJSON *nextEvents, *nextSummaryCounters, *summaryEvent;
    double now;

    LD_ASSERT(context);
    LD_ASSERT(result);

    nextEvents          = NULL;
    nextSummaryCounters = NULL;
    *result             = NULL;
    summaryEvent        = NULL;

    LDi_getUnixMilliseconds(&now);

    LDi_mutex_lock(&context->lock);

    if (LDCollectionGetSize(context->events) == 0 &&
        LDCollectionGetSize(context->summaryCounters) == 0)
    {
        LDi_mutex_unlock(&context->lock);

        /* succesful but no events to send */

        return true;
    }

    if (!(nextEvents = LDNewArray())) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        LDi_mutex_unlock(&context->lock);

        return false;
    }

    if (context->summaryStart != 0) {
        if (!(nextSummaryCounters = LDNewObject())) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            LDi_mutex_unlock(&context->lock);

            LDJSONFree(nextEvents);

            return false;
        }

        if (!(summaryEvent = LDi_prepareSummaryEvent(context, now))) {
            LD_LOG(LD_LOG_ERROR, "failed to prepare summary");

            LDi_mutex_unlock(&context->lock);

            LDJSONFree(nextEvents);
            LDJSONFree(nextSummaryCounters);

            return false;
        }

        LDArrayPush(context->events, summaryEvent);

        LDJSONFree(context->summaryCounters);

        context->summaryStart    = 0;
        context->summaryCounters = nextSummaryCounters;
    }

    *result = context->events;

    context->events = nextEvents;

    LDi_mutex_unlock(&context->lock);

    return true;
}

struct LDJSON *
LDi_valueToJSON(
    const void *const value,
    const LDJSONType  valueType
) {
    struct LDJSON *tmp;

    LD_ASSERT(value);

    switch (valueType) {
        case LDNull:   tmp = LDJSONDuplicate((struct LDJSON *)value);  break;
        case LDBool:   tmp = LDNewBool(*(bool *)value);                break;
        case LDText:   tmp = LDNewText((const char *)value);           break;
        case LDNumber: tmp = LDNewNumber(*(double *)value);            break;
        /* LDNull is actually used to represent these types for now */
        case LDObject: LD_ASSERT(false);                               break;
        case LDArray:  LD_ASSERT(false);                               break;
    }

    return tmp;
}

struct LDJSON *
LDi_newFeatureRequestEvent(
    struct EventProcessor *const    context,
    const char *const               flagKey,
    const struct LDUser *const      user,
    const LDJSONType                variationType,
    const void *const               fallbackValue,
    const void *const               actualValue,
    const struct LDStoreNode *const node,
    const bool                      detailed,
    const double                    now
) {
    struct LDJSON *event, *tmp;

    LD_ASSERT(context);
    LD_ASSERT(flagKey);
    LD_ASSERT(user);
    LD_ASSERT(fallbackValue);

    tmp   = NULL;
    event = NULL;

    if (!(event = LDi_newBaseEvent("feature", now))) {
        return NULL;
    }

    if (!LDi_addUserInfoToEvent(context, event, user)) {
        LD_LOG(LD_LOG_ERROR,
            "LDi_newFeatureRequestEvent failed adding user info");

        return NULL;
    }

    if (!(tmp = LDNewText(flagKey))) {
        return NULL;
    }

    if (!LDObjectSetKey(event, "key", tmp)) {
        return NULL;
    } else {
        tmp = NULL;
    }

    if (!(tmp = LDi_valueToJSON(actualValue, variationType))) {
        return NULL;
    }

    if (!LDObjectSetKey(event, "value", tmp)) {
        return NULL;
    } else {
        tmp = NULL;
    }

    if (!(tmp = LDi_valueToJSON(fallbackValue, variationType))) {
        return NULL;
    }

    if (!LDObjectSetKey(event, "default", tmp)) {
        return NULL;
    } else {
        tmp = NULL;
    }

    if (node) {
        if (node->flag.variation != -1) {
            if (!(tmp = LDNewNumber(node->flag.variation))) {
                return NULL;
            }

            if (!LDObjectSetKey(event, "variation", tmp)) {
                return NULL;
            } else {
                tmp = NULL;
            }
        }

        if (!(tmp = LDNewNumber(LDi_getFlagVersion(&node->flag)))) {
            return NULL;
        }

        if (!LDObjectSetKey(event, "version", tmp)) {
            return NULL;
        } else {
            tmp = NULL;
        }

        if (detailed && node->flag.reason) {
            if (!(tmp = LDJSONDuplicate(node->flag.reason))) {
                return NULL;
            }

            if (!LDObjectSetKey(event, "reason", tmp)) {
                return NULL;
            } else {
                tmp = NULL;
            }
        }
    }

    if (user->anonymous) {
        if (!(tmp = LDNewText("anonymousUser"))) {
            LDJSONFree(event);

            return NULL;
        }

        if (!LDObjectSetKey(event, "contextKind", tmp)) {
            LDJSONFree(tmp);
            LDJSONFree(event);

            return NULL;
        }
    }

    return event;
}

bool
LDi_summarizeEvent(
    struct EventProcessor *const    context,
    const char *const               flagKey,
    const struct LDStoreNode *const node,
    const LDJSONType                variationType,
    const void *const               fallbackValue,
    const void *const               actualValue
) {
    int status;
    char keyText[128];
    struct LDJSON *tmp, *entry, *flagContext, *counters;
    bool success;

    LD_ASSERT(context);
    LD_ASSERT(flagKey);

    tmp         = NULL;
    entry       = NULL;
    flagContext = NULL;
    counters    = NULL;
    success     = false;

    /* prepare summary key */
    if (node == NULL) {
        status = snprintf(keyText, sizeof(keyText),
            "unknown");
    } else {
        status = snprintf(keyText, sizeof(keyText),
            "%d %d", LDi_getFlagVersion(&node->flag), node->flag.variation);
    }

    if (status < 0) {
        LD_LOG(LD_LOG_ERROR, "LDi_summarizeEvent failed to create key");

        return false;
    }

    if (context->summaryStart == 0) {
        double now;

        LDi_getUnixMilliseconds(&now);

        context->summaryStart = now;
    }

    if (!(flagContext = LDObjectLookup(context->summaryCounters, flagKey))) {
        if (!(flagContext = LDNewObject())) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            goto cleanup;
        }

        if (fallbackValue) {
            if (!(tmp = LDi_valueToJSON(fallbackValue, variationType))) {
                LD_LOG(LD_LOG_ERROR, "alloc error");

                LDJSONFree(flagContext);

                goto cleanup;
            }

            if (!LDObjectSetKey(flagContext, "default", tmp)) {
                LD_LOG(LD_LOG_ERROR, "alloc error");

                LDJSONFree(tmp);
                LDJSONFree(flagContext);

                goto cleanup;
            }
        }

        if (!(tmp = LDNewObject())) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            LDJSONFree(flagContext);

            goto cleanup;
        }

        if (!LDObjectSetKey(flagContext, "counters", tmp)) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            LDJSONFree(tmp);
            LDJSONFree(flagContext);

            goto cleanup;
        }

        if (!LDObjectSetKey(context->summaryCounters, flagKey, flagContext)) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            LDJSONFree(flagContext);

            goto cleanup;
        }
    }

    counters = LDObjectLookup(flagContext, "counters");
    LD_ASSERT(counters);
    LD_ASSERT(LDJSONGetType(counters) == LDObject);

    if (!(entry = LDObjectLookup(counters, keyText))) {
        if (!(entry = LDNewObject())) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            goto cleanup;
        }

        if (!(tmp = LDNewNumber(1))) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            LDJSONFree(entry);

            goto cleanup;
        }

        if (!LDObjectSetKey(entry, "count", tmp)) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            LDJSONFree(tmp);
            LDJSONFree(entry);

            goto cleanup;
        }

        if (!(tmp = LDi_valueToJSON(actualValue, variationType))) {
            goto cleanup;
        }

        if (!LDObjectSetKey(entry, "value", tmp)) {
            goto cleanup;
        } else {
            tmp = NULL;
        }

        if (node) {
            if (!(tmp = LDNewNumber(LDi_getFlagVersion(&node->flag)))) {
                LD_LOG(LD_LOG_ERROR, "alloc error");

                LDJSONFree(entry);

                goto cleanup;
            }

            if (!LDObjectSetKey(entry, "version", tmp)) {
                LD_LOG(LD_LOG_ERROR, "alloc error");

                LDJSONFree(tmp);
                LDJSONFree(entry);

                goto cleanup;
            }

            if (node->flag.variation != -1) {
                if (!(tmp = LDNewNumber(node->flag.variation))) {
                    LD_LOG(LD_LOG_ERROR, "alloc error");

                    LDJSONFree(entry);

                    goto cleanup;
                }

                if (!LDObjectSetKey(entry, "variation", tmp)) {
                    LD_LOG(LD_LOG_ERROR, "alloc error");

                    LDJSONFree(tmp);
                    LDJSONFree(entry);

                    goto cleanup;
                }
            }
        } else {
            if (!(tmp = LDNewBool(true))) {
                LD_LOG(LD_LOG_ERROR, "alloc error");

                LDJSONFree(entry);

                goto cleanup;
            }

            if (!LDObjectSetKey(entry, "unknown", tmp)) {
                LD_LOG(LD_LOG_ERROR, "alloc error");

                LDJSONFree(tmp);
                LDJSONFree(entry);

                goto cleanup;
            }
        }

        if (!LDObjectSetKey(counters, keyText, entry)) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            LDJSONFree(entry);

            goto cleanup;
        }
    } else {
        bool status;
        tmp = LDObjectLookup(entry, "count");
        LD_ASSERT(tmp);
        status = LDSetNumber(tmp, LDGetNumber(tmp) + 1);
        LD_ASSERT(status);
    }

    success = true;

  cleanup:
    return success;
}

static bool
shouldGenerateFeatureEvent(const struct LDStoreNode *const node,
    const double now)
{
    return node &&
        (node->flag.trackEvents || node->flag.debugEventsUntilDate > now);
}

bool
LDi_processEvalEvent(
    struct EventProcessor *const    context,
    const struct LDUser *const      user,
    const char *const               flagKey,
    const LDJSONType                valueType,
    const struct LDStoreNode *const node,
    const void *const               actualValue,
    const void *const               fallback,
    const bool                      detailed
) {
    struct LDJSON *featureEvent;
    double now;

    LD_ASSERT(context);
    LD_ASSERT(user);
    LD_ASSERT(flagKey);
    LD_ASSERT(actualValue);
    LD_ASSERT(fallback);

    featureEvent = NULL;

    LDi_getUnixMilliseconds(&now);

    if (shouldGenerateFeatureEvent(node, now)) {
        featureEvent = LDi_newFeatureRequestEvent(context, flagKey, user,
            valueType, fallback, actualValue, node, detailed, now);

        if (featureEvent == NULL) {
            LD_LOG(LD_LOG_ERROR, "failed to create feature event");

            return false;
        }
    }

    LDi_mutex_lock(&context->lock);

    if (!LDi_summarizeEvent(context, flagKey, node, valueType, fallback,
        actualValue))
    {
        LDi_addEvent(context, featureEvent);

        LDJSONFree(featureEvent);

        LDi_mutex_unlock(&context->lock);

        return false;
    }

    if (featureEvent) {
        LDi_addEvent(context, featureEvent);
    }

    LDi_mutex_unlock(&context->lock);

    return true;
}
