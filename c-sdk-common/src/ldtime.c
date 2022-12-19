#include "ldtime.h"
#include "utility.h"
#include "assertion.h"
#include <launchdarkly/json.h>

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

LDBoolean
LDTimestamp_InitNow(struct LDTimestamp *timestamp) {
    LD_ASSERT(timestamp);

    if (!LDi_getUnixMilliseconds(&timestamp->ms)) {
        timestamp->ms = -1;
        return LDBooleanFalse;
    }

    return LDBooleanTrue;
}

LDBoolean
LDTimestamp_IsInvalid(const struct LDTimestamp *timestamp) {
    LD_ASSERT(timestamp);
    return (LDBoolean) (timestamp->ms == -1);
}

void
LDTimestamp_InitZero(struct LDTimestamp *timestamp) {
    LD_ASSERT(timestamp);
    timestamp->ms = 0;
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
    LD_ASSERT(a);
    LD_ASSERT(b);
    return (LDBoolean) (a->ms < b->ms);
}

LDBoolean
LDTimestamp_Equal(const struct LDTimestamp *a, const struct LDTimestamp *b) {
    LD_ASSERT(a);
    LD_ASSERT(b);
    return (LDBoolean) (a->ms == b->ms);
}

double
LDTimestamp_AsUnixMillis(const struct LDTimestamp *timestamp) {
    LD_ASSERT(timestamp);
    if (LDTimestamp_IsInvalid(timestamp)) {
        return -1;
    }
    return timestamp->ms;
}


struct LDJSON *
LDTimestamp_MarshalUnixMillis(const struct LDTimestamp *timestamp) {
    LD_ASSERT(timestamp);
    if (LDTimestamp_IsInvalid(timestamp)) {
        return LDNewNumber(-1);
    }
    return LDNewNumber(timestamp->ms);
}
