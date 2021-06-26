#pragma once

#include <launchdarkly/boolean.h>
#include <launchdarkly/json.h>

struct LDFlag
{
    char *         key;
    struct LDJSON *value;
    int            version;
    int            flagVersion;
    int            variation;
    LDBoolean      trackEvents;
    struct LDJSON *reason;
    double         debugEventsUntilDate;
    LDBoolean      deleted;
};

LDBoolean
LDi_flag_parse(
    struct LDFlag *const       result,
    const char *const          key,
    const struct LDJSON *const raw);

struct LDJSON *
LDi_flag_to_json(struct LDFlag *const flag);

void
LDi_flag_destroy(struct LDFlag *const flag);
