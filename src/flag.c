#include <launchdarkly/memory.h>

#include "assertion.h"
#include "flag.h"

bool
LDi_flag_parse(struct LDFlag *const result, const char *const key,
    const struct LDJSON *const raw)
{
    const struct LDJSON *tmp;

    LD_ASSERT(result);
    LD_ASSERT(raw);

    result->key         = NULL;
    result->value       = NULL;
    result->reason      = NULL;
    result->deleted     = false;
    result->version     = -1;
    result->flagVersion = -1;

    if (LDJSONGetType(raw) != LDObject) {
        LD_LOG(LD_LOG_ERROR, "LDi_flag_parse not an object");

        goto error;
    }

    /* key */
    if (key) {
        if (!(result->key = LDStrDup(key))) {
            LD_LOG(LD_LOG_ERROR, "LDi_flag_parse failed to duplicate key");

            goto error;
        }
    } else {
        if (!(tmp = LDObjectLookup(raw, "key"))) {
            LD_LOG(LD_LOG_ERROR, "LDi_flag_parse expected key");

            goto error;
        }

        if (LDJSONGetType(tmp) != LDText) {
            LD_LOG(LD_LOG_ERROR, "LDi_flag_parse key is not text");

            goto error;
        }

        if (!(result->key = LDStrDup(LDGetText(tmp)))) {
            LD_LOG(LD_LOG_ERROR, "LDi_flag_parse failed to duplicate key");

            goto error;
        }
    }

    /* value */
    if (!(tmp = LDObjectLookup(raw, "value"))) {
        LD_LOG(LD_LOG_ERROR, "LDi_flag_parse expected value");

        goto error;
    }

    if (!(result->value = LDJSONDuplicate(tmp))) {
        LD_LOG(LD_LOG_ERROR, "LDi_flag_parse failed to duplicate value");

        goto error;
    }

    /* flagVersion */
    if ((tmp = LDObjectLookup(raw, "flagVersion"))) {
        if (LDJSONGetType(tmp) != LDNumber) {
            LD_LOG(LD_LOG_ERROR, "LDi_flag_parse flagVersion is not a number");

            goto error;
        }

        result->flagVersion = LDGetNumber(tmp);
    }

    /* version */
    if ((tmp = LDObjectLookup(raw, "version"))) {
        if (LDJSONGetType(tmp) != LDNumber) {
            LD_LOG(LD_LOG_ERROR, "LDi_flag_parse version is not a number");

            goto error;
        }

        result->version = LDGetNumber(tmp);
    }

    /* variaton */
    if (!(tmp = LDObjectLookup(raw, "variation"))) {
        LD_LOG(LD_LOG_ERROR, "LDi_flag_parse expected variation");

        goto error;
    }

    if (LDJSONGetType(tmp) == LDNumber) {
        result->variation = LDGetNumber(tmp);
    } else if (LDJSONGetType(tmp) == LDNull) {
        result->variation = -1;
    } else {
        LD_LOG(LD_LOG_ERROR,
            "LDi_flag_parse variation is not a number or null");

        goto error;
    }

    /* trackEvents */
    if ((tmp = LDObjectLookup(raw, "trackEvents"))) {
        if (LDJSONGetType(tmp) != LDBool) {
            LD_LOG(LD_LOG_ERROR, "LDi_flag_parse trackEvents is not boolean");

            goto error;
        }

        result->trackEvents = LDGetBool(tmp);
    } else {
        result->trackEvents = false;
    }

    /* debugEventsUntilDate */
    if ((tmp = LDObjectLookup(raw, "debugEventsUntilDate"))) {
        if (LDJSONGetType(tmp) != LDNumber) {
            LD_LOG(LD_LOG_ERROR,
                "LDi_flag_parse debugEventsUntilDate not a number");

            goto error;
        }

        result->debugEventsUntilDate = LDGetNumber(tmp);
    } else {
        result->debugEventsUntilDate = 0;
    }

    /* reason */
    if ((tmp = LDObjectLookup(raw, "reason"))) {
        if (LDJSONGetType(tmp) != LDObject) {
            LD_LOG(LD_LOG_ERROR, "LDi_flag_parse reason is not an object");

            goto error;
        }

        if (!(result->reason = LDJSONDuplicate(tmp))) {
            LD_LOG(LD_LOG_ERROR, "LDi_flag_parse failed to duplicate reason");

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
