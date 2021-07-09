#pragma once

/* exposed for testing */

#include <launchdarkly/json.h>

#include "concurrency.h"
#include "event_processor.h"

struct EventProcessor
{
    ld_mutex_t             lock;
    struct LDJSON *        events;          /* Array of Objects */
    struct LDJSON *        summaryCounters; /* Object */
    double                 summaryStart;
    double                 lastUserKeyFlush;
    double                 lastServerTime;
    const struct LDConfig *config;
};

void
LDi_addEvent(struct EventProcessor *const context, struct LDJSON *const event);

struct LDJSON *
LDi_newBaseEvent(const char *const kind, const double now);

LDBoolean
LDi_addUserInfoToEvent(
    const struct EventProcessor *const context,
    struct LDJSON *const               event,
    const struct LDUser *const         user);

struct LDJSON *
LDi_newIdentifyEvent(
    const struct EventProcessor *const context,
    const struct LDUser *const         user,
    const double                       now);

struct LDJSON *
LDi_newCustomEvent(
    const struct EventProcessor *const context,
    const struct LDUser *const         user,
    const char *const                  key,
    struct LDJSON *const               data,
    const double                       metric,
    const LDBoolean                    hasMetric,
    const double                       now);

struct LDJSON *
LDi_newAliasEvent(
    const struct LDUser *const currentUser,
    const struct LDUser *const previousUser,
    const double               now);

struct LDJSON *
LDi_objectToArray(const struct LDJSON *const object);

struct LDJSON *
LDi_prepareSummaryEvent(struct EventProcessor *const context, const double now);

struct LDJSON *
LDi_valueToJSON(const void *const value, const LDJSONType valueType);

struct LDJSON *
LDi_newFeatureRequestEvent(
    struct EventProcessor *const    context,
    const char *const               flagKey,
    const struct LDUser *const      user,
    const LDJSONType                variationType,
    const void *const               fallbackValue,
    const void *const               actualValue,
    const struct LDStoreNode *const node,
    const LDBoolean                 detailed,
    const double                    now);

LDBoolean
LDi_summarizeEvent(
    struct EventProcessor *const    context,
    const char *const               flagKey,
    const struct LDStoreNode *const node,
    const LDJSONType                variationType,
    const void *const               fallbackValue,
    const void *const               actualValue);
