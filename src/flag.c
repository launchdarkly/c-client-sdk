#include <launchdarkly/memory.h>

#include "assertion.h"
#include "flag.h"

LDBoolean
LDi_flag_parse(
    struct LDFlag *const       result,
    const char *const          key,
    const struct LDJSON *const raw)
{
    const struct LDJSON *tmp;

    LD_ASSERT(result);
    LD_ASSERT(raw);

    result->key         = NULL;
    result->value       = NULL;
    result->reason      = NULL;
    result->deleted     = LDBooleanFalse;
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

    /* variation */
    if (!(tmp = LDObjectLookup(raw, "variation"))) {
        LD_LOG(LD_LOG_ERROR, "LDi_flag_parse expected variation");

        goto error;
    }

    if (LDJSONGetType(tmp) == LDNumber) {
        result->variation = LDGetNumber(tmp);
    } else if (LDJSONGetType(tmp) == LDNull) {
        result->variation = -1;
    } else {
        LD_LOG(
            LD_LOG_ERROR, "LDi_flag_parse variation is not a number or null");

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
        result->trackEvents = LDBooleanFalse;
    }


    /* trackReason */
    if ((tmp = LDObjectLookup(raw, "trackReason"))) {
        if (LDJSONGetType(tmp) != LDBool) {
            LD_LOG(LD_LOG_ERROR, "LDi_flag_parse trackReason is not boolean");

            goto error;
        }

        result->trackReason = LDGetBool(tmp);
    } else {
        result->trackReason = LDBooleanFalse;
    }

    /* debugEventsUntilDate */
    if ((tmp = LDObjectLookup(raw, "debugEventsUntilDate"))) {
        if (LDJSONGetType(tmp) != LDNumber) {
            LD_LOG(
                LD_LOG_ERROR,
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

    /* deleted */
    if ((tmp = LDObjectLookup(raw, "deleted"))) {
        if (LDJSONGetType(tmp) != LDBool) {
            LD_LOG(LD_LOG_ERROR, "LDi_flag_parse deleted is not a boolean");

            goto error;
        }

        result->deleted = LDGetBool(tmp);
    }

    return LDBooleanTrue;

error:
    LDFree(result->key);
    LDJSONFree(result->value);
    LDJSONFree(result->reason);

    return LDBooleanFalse;
}

struct LDJSON *
LDi_flag_to_json(struct LDFlag *const flag)
{
    struct LDJSON *result, *tmp;

    LD_ASSERT(flag);

    result = NULL;
    tmp    = NULL;

    if (!(result = LDNewObject())) {
        goto error;
    }

    if (!(tmp = LDNewText(flag->key))) {
        goto error;
    }

    if (!LDObjectSetKey(result, "key", tmp)) {
        goto error;
    }
    tmp = NULL;

    if (!(tmp = LDJSONDuplicate(flag->value))) {
        goto error;
    }

    if (!LDObjectSetKey(result, "value", tmp)) {
        goto error;
    }
    tmp = NULL;

    if (flag->version >= 0) {
        if (!(tmp = LDNewNumber(flag->version))) {
            goto error;
        }

        if (!LDObjectSetKey(result, "version", tmp)) {
            goto error;
        }
        tmp = NULL;
    }

    if (flag->flagVersion >= 0) {
        if (!(tmp = LDNewNumber(flag->flagVersion))) {
            goto error;
        }

        if (!LDObjectSetKey(result, "flagVersion", tmp)) {
            goto error;
        }
        tmp = NULL;
    }

    if (!(tmp = LDNewNumber(flag->variation))) {
        goto error;
    }

    if (!LDObjectSetKey(result, "variation", tmp)) {
        goto error;
    }
    tmp = NULL;

    if (flag->trackEvents) {
        if (!(tmp = LDNewBool(LDBooleanTrue))) {
            goto error;
        }

        if (!LDObjectSetKey(result, "trackEvents", tmp)) {
            goto error;
        }
        tmp = NULL;
    }

    if (flag->trackReason) {
        if (!(tmp = LDNewBool(LDBooleanTrue))) {
            goto error;
        }

        if (!LDObjectSetKey(result, "trackReason", tmp)) {
            goto error;
        }
        tmp = NULL;
    }

    if (flag->reason) {
        if (!(tmp = LDJSONDuplicate(flag->reason))) {
            goto error;
        }

        if (!LDObjectSetKey(result, "reason", tmp)) {
            goto error;
        }
        tmp = NULL;
    }

    if (flag->debugEventsUntilDate != 0) {
        if (!(tmp = LDNewNumber(flag->debugEventsUntilDate))) {
            goto error;
        }

        if (!LDObjectSetKey(result, "debugEventsUntilDate", tmp)) {
            goto error;
        }
        tmp = NULL;
    }

    if (flag->deleted) {
        if (!(tmp = LDNewBool(LDBooleanTrue))) {
            goto error;
        }

        if (!LDObjectSetKey(result, "deleted", tmp)) {
            goto error;
        }
        tmp = NULL;
    }

    return result;

error:
    LDJSONFree(result);
    LDJSONFree(tmp);

    return NULL;
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
