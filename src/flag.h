#pragma once

#include <stdbool.h>

#include <launchdarkly/json.h>

struct LDFlag {
    char *key;
    struct LDJSON *value;
    unsigned int version;
    unsigned int variation;
    bool trackEvents;
    struct LDJSON *reason;
    double debugEventsUntilDate;
    bool deleted;
};

bool LDi_flag_parse(struct LDFlag *const result,
    const struct LDJSON *const raw);

void LDi_flag_destroy(struct LDFlag *const flag);
