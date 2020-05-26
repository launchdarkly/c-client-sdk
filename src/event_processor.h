#pragma once

#include <launchdarkly/json.h>

#include "ldapi.h"

struct EventProcessor;

struct EventProcessor *
LDi_newEventProcessor(
    const LDConfig *const config
);

void
LDi_freeEventProcessor(
    struct EventProcessor *const context
);

bool
LDi_identify(
    struct EventProcessor *const context,
    const LDUser *const          user
);

bool
LDi_track(
    struct EventProcessor *const context,
    const LDUser *const          user,
    const char *const            key,
    struct LDJSON *const         data,
    const double                 metric,
    const bool                   hasMetric
);

bool
LDi_bundleEventPayload(
    struct EventProcessor *const context,
    struct LDJSON **const        result
);
