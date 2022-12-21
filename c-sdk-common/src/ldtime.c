#include "ldtime.h"
#include "utility.h"
#include "assertion.h"
#include <launchdarkly/json.h>

#define LDTIMESTAMP_EMPTY -1

LDBoolean
LDTimer_Reset(struct LDTimer *timer) {
    LD_ASSERT(timer);
    return LDi_getMonotonicMilliseconds(&timer->ms);
}

LDBoolean
LDTimer_Elapsed(const struct LDTimer *timer, double *out_elapsedMs) {
    double nowMs;
    LD_ASSERT(timer);

    if (!LDi_getMonotonicMilliseconds(&nowMs)) {
        return LDBooleanFalse;
    }

    *out_elapsedMs = nowMs - timer->ms;
    return LDBooleanTrue;
}

void
LDTimestamp_InitEmpty(struct LDTimestamp *timestamp)
{
    LD_ASSERT(timestamp);
    timestamp->ms = LDTIMESTAMP_EMPTY;
}

LDBoolean
LDTimestamp_InitNow(struct LDTimestamp *timestamp) {
    LD_ASSERT(timestamp);

    if (!LDi_getUnixMilliseconds(&timestamp->ms)) {
        LDTimestamp_InitEmpty(timestamp);
        return LDBooleanFalse;
    }

    return LDBooleanTrue;
}

LDBoolean
LDTimestamp_IsEmpty(const struct LDTimestamp *timestamp)
{
    LD_ASSERT(timestamp);
    return (LDBoolean) (timestamp->ms == LDTIMESTAMP_EMPTY);
}

LDBoolean
LDTimestamp_IsSet(const struct LDTimestamp *timestamp)
{
    LD_ASSERT(timestamp);
    return (LDBoolean) (timestamp->ms >= 0);
}

void
LDTimestamp_InitUnixSeconds(struct LDTimestamp *timestamp, time_t unixSeconds) {
    LD_ASSERT(unixSeconds >= 0);
    timestamp->ms = 1000.0 * (double) unixSeconds;
}

void
LDTimestamp_InitUnixMillis(struct LDTimestamp *timestamp, double unixMilliseconds) {
    LD_ASSERT(unixMilliseconds >= 0);
    timestamp->ms = unixMilliseconds;
}


LDBoolean
LDTimestamp_Before(const struct LDTimestamp *a, const struct LDTimestamp *b) {
    LD_ASSERT(LDTimestamp_IsSet(a));
    LD_ASSERT(LDTimestamp_IsSet(b));
    return (LDBoolean) (a->ms < b->ms);
}

LDBoolean
LDTimestamp_After(const struct LDTimestamp *a, const struct LDTimestamp *b)
{
    LD_ASSERT(LDTimestamp_IsSet(a));
    LD_ASSERT(LDTimestamp_IsSet(b));
    return (LDBoolean) (a->ms > b->ms);
}

LDBoolean
LDTimestamp_Equal(const struct LDTimestamp *a, const struct LDTimestamp *b) {
    LD_ASSERT(LDTimestamp_IsSet(a));
    LD_ASSERT(LDTimestamp_IsSet(b));
    return (LDBoolean) (a->ms == b->ms);
}

double
LDTimestamp_AsUnixMillis(const struct LDTimestamp *timestamp) {
    LD_ASSERT(LDTimestamp_IsSet(timestamp));
    return timestamp->ms;
}


struct LDJSON *
LDTimestamp_MarshalUnixMillis(const struct LDTimestamp *timestamp) {
    LD_ASSERT(timestamp);
    if (LDTimestamp_IsEmpty(timestamp)) {
        return LDNewNull();
    }
    return LDNewNumber(timestamp->ms);
}
