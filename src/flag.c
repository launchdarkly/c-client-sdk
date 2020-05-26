#include <launchdarkly/memory.h>

#include "assertion.h"
#include "flag.h"

bool
LDi_flag_parse(struct LDFlag *const result, const struct LDJSON *const raw)
{
    const struct LDJSON *tmp;

    LD_ASSERT(result);
    LD_ASSERT(raw);

    result->key     = NULL;
    result->value   = NULL;
    result->reason  = NULL;
    result->deleted = false;

    if (LDJSONGetType(raw) != LDObject) {
        goto error;
    }

    /* key */
    if (!(tmp = LDObjectLookup(raw, "key"))) {
        goto error;
    }

    if (LDJSONGetType(tmp) != LDText) {
        goto error;
    }

    if (!(result->key = LDStrDup(LDGetText(tmp)))) {
        goto error;
    }

    /* version */
    if (!(tmp = LDObjectLookup(raw, "version"))) {
        goto error;
    }

    if (LDJSONGetType(tmp) != LDNumber) {
        goto error;
    }

    result->version = LDGetNumber(tmp);

    /* variaton */
    if (!(tmp = LDObjectLookup(raw, "variation"))) {
        goto error;
    }

    if (LDJSONGetType(tmp) != LDNumber) {
        goto error;
    }

    result->variation = LDGetNumber(tmp);

    /* trackEvents */
    if ((tmp = LDObjectLookup(raw, "trackEvents"))) {
        if (LDJSONGetType(tmp) != LDNumber) {
            goto error;
        }

        result->trackEvents = LDGetBool(tmp);
    } else {
        result->trackEvents = false;
    }

    /* debugEventsUntilDate */
    if ((tmp = LDObjectLookup(raw, "debugEventsUntilDate"))) {
        if (LDJSONGetType(tmp) != LDNumber) {
            goto error;
        }

        result->debugEventsUntilDate = LDGetNumber(tmp);
    } else {
        result->debugEventsUntilDate = 0;
    }

    /* reason */
    if ((tmp = LDObjectLookup(raw, "reason"))) {
        if (LDJSONGetType(tmp) != LDObject) {
            goto error;
        }

        if (!(result->reason = LDJSONDuplicate(tmp))) {
            goto error;
        }
    } else {
        result->reason = NULL;
    }

    return true;

  error:
    LDFree(result->key);
    LDJSONFree(result->value);
    LDJSONFree(result->reason);

    return false;
}

void
LDi_flag_destroy(struct LDFlag *const flag)
{
    if (flag) {
        LDFree(flag->key);
        LDJSONFree(flag->value);
        LDJSONFree(flag->reason);
    }
}
