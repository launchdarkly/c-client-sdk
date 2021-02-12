#pragma once

#include <launchdarkly/api.h>
#include <launchdarkly/json.h>

#include "store.h"

struct EventProcessor;

struct EventProcessor *
LDi_newEventProcessor(const struct LDConfig *const config);

void
LDi_freeEventProcessor(struct EventProcessor *const context);

bool
LDi_identify(
    struct EventProcessor *const context, const struct LDUser *const user);

bool
LDi_track(
    struct EventProcessor *const context,
    const struct LDUser *const   user,
    const char *const            key,
    struct LDJSON *const         data,
    const double                 metric,
    const bool                   hasMetric);

bool
LDi_alias(
    struct EventProcessor *const context,
    const struct LDUser *const   currentUser,
    const struct LDUser *const   previousUser);

bool
LDi_bundleEventPayload(
    struct EventProcessor *const context, struct LDJSON **const result);

bool
LDi_processEvalEvent(
    struct EventProcessor *const    context,
    const struct LDUser *const      user,
    const char *const               flagKey,
    const LDJSONType                valueType,
    const struct LDStoreNode *const node,
    const void *const               actualValue,
    const void *const               fallback,
    const bool                      detailed);
