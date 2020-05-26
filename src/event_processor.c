#include "event_processor.h"
#include "event_processor_internal.h"
#include "utility.h"
#include "ldinternal.h"

struct EventProcessor *
LDi_newEventProcessor(const LDConfig *const config)
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
    const LDUser *const                user
) {
    struct LDJSON *tmp;

    LD_ASSERT(context);
    LD_ASSERT(event);
    LD_ASSERT(user);

    if (context->config->inlineUsersInEvents) {
        /* commenting out for now because no such function */
        /*
        if (!(tmp = LDUserToJSON(context->config, user, true))) {
            LD_LOG(LD_LOG_ERROR, "alloc error");

            return false;
        }
        */

        tmp = NULL;

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
    const LDUser *const                user,
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

    /* comment out for now because no such function */
    /*
    if (!(tmp = LDUserToJSON(context->config, user, true))) {
        LD_LOG(LD_LOG_ERROR, "alloc error");

        LDJSONFree(event);

        return NULL;
    }
    */
    tmp = NULL;

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
    const LDUser *const          user
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
    const LDUser *const                user,
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

    return event;

  error:
    LDJSONFree(event);

    return NULL;
}

bool
LDi_track(
    struct EventProcessor *const context,
    const LDUser *const          user,
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

    *result = context->events;

    LDJSONFree(context->summaryCounters);

    context->summaryStart    = 0;
    context->events          = nextEvents;
    context->summaryCounters = nextSummaryCounters;

    LDi_mutex_unlock(&context->lock);

    return true;
}
