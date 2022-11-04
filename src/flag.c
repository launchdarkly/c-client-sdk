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
    result->version     = -1;
    result->flagVersion = -1;
    result->variation = -1;
    result->trackEvents = LDBooleanFalse;
    result->trackReason = LDBooleanFalse;
    result->reason      = NULL;
    result->debugEventsUntilDate = 0;
    result->deleted     = LDBooleanFalse;

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

    /* value; required */
    if ((tmp = LDObjectLookup(raw, "value"))) {
        if (!(result->value = LDJSONDuplicate(tmp))) {
            LD_LOG(LD_LOG_ERROR, "LDi_flag_parse failed to duplicate value");

            goto error;
        }
    } else {
        LD_LOG(LD_LOG_ERROR, "LDi_flag_parse expected value");

        goto error;
    }

    /* version; required */
    if ((tmp = LDObjectLookup(raw, "version"))) {
        if (LDJSONGetType(tmp) != LDNumber) {
            LD_LOG(LD_LOG_ERROR, "LDi_flag_parse version is not a number");

            goto error;
        }

        result->version = (int) LDGetNumber(tmp);
    } else {
        LD_LOG(LD_LOG_ERROR, "LDi_flag_parse expected version");

        goto error;
    }

    /* deleted; optional (not part of data format - added by client when serializing) */
    if ((tmp = LDObjectLookup(raw, "deleted"))) {
        if (LDJSONGetType(tmp) != LDBool) {
            LD_LOG(LD_LOG_ERROR, "LDi_flag_parse deleted is not a boolean");

            goto error;
        }

        result->deleted = LDGetBool(tmp);
    }

    /* flagVersion; optional */
    if ((tmp = LDObjectLookup(raw, "flagVersion"))) {
        if (LDJSONGetType(tmp) == LDNumber) {
            result->flagVersion = (int) LDGetNumber(tmp);
        }
    }

    /* variation; optional */
    if ((tmp = LDObjectLookup(raw, "variation"))) {
        if (LDJSONGetType(tmp) == LDNumber) {
            result->variation = (int) LDGetNumber(tmp);
        }
    }


    /* trackEvents; optional */
    if ((tmp = LDObjectLookup(raw, "trackEvents"))) {
        if (LDJSONGetType(tmp) == LDBool) {
            result->trackEvents = LDGetBool(tmp);
        }
    }


    /* trackReason; optional */
    if ((tmp = LDObjectLookup(raw, "trackReason"))) {
        if (LDJSONGetType(tmp) == LDBool) {
            result->trackReason = LDGetBool(tmp);
        }
    }

    /* debugEventsUntilDate; optional */
    if ((tmp = LDObjectLookup(raw, "debugEventsUntilDate"))) {
        if (LDJSONGetType(tmp) == LDNumber) {
            result->debugEventsUntilDate = LDGetNumber(tmp);
        }
    }

    /* reason; optional */
    if ((tmp = LDObjectLookup(raw, "reason"))) {
        if (LDJSONGetType(tmp) == LDObject) {
            if (!(result->reason = LDJSONDuplicate(tmp))) {
                LD_LOG(LD_LOG_ERROR, "LDi_flag_parse failed to duplicate reason");

                goto error;
            }
        }
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
