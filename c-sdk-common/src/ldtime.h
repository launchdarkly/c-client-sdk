#pragma once
#include <launchdarkly/boolean.h>
#include <time.h>

/* LDTimer allows for measurement of an elapsed duration between
 * two points in time, using a monotonic clock. */
struct LDTimer {
    double ms;
};

/* LDTimestamp allows for retrieving and comparing wall-clock timestamps.
 * Additionally, an LDTimestamp can be marshalled directly to LDJSON. */
struct LDTimestamp {
    double ms;
};

/* Reset an LDTimer. Once an LDTimer has been reset, LDTimer_Elapsed may be called
 * to retrieve the elapsed milliseconds between the call to LDTimer_Elapsed and LDTimer_Reset.
 * Returns false if obtaining a monotonic timestamp failed. */
LDBoolean
LDTimer_Reset(struct LDTimer *timer);

/* Retrieve the elapsed time between a previous call to LDTimer_Reset and now, in milliseconds.
 * Returns false if obtaining a monotonic timestamp failed. */
LDBoolean
LDTimer_Elapsed(const struct LDTimer *timer, double *out_elapsedMs);

/* Initialize a timestamp with the current wall time.
 * Obtaining a timestamp may fail, which can be detected immediately by a false return value,
 * or later using LDTimestamp_IsInitialized. */
LDBoolean
LDTimestamp_InitNow(struct LDTimestamp *timestamp);

/* Initialize a timestamp to 0. */
void
LDTimestamp_InitZero(struct LDTimestamp *timestamp);

/* Initialize a timestamp from a time_t value. The value must be >= 0. */
void
LDTimestamp_InitUnixSeconds(struct LDTimestamp *timestamp, time_t unixSeconds);

/* Initialize a timestamp from a double representing Unix milliseconds. The value must be >= 0. */
void
LDTimestamp_InitUnixMillis(struct LDTimestamp *timestamp, double unixMilliseconds);
/* Returns true if timestamp is known to be invalid. */
LDBoolean
LDTimestamp_IsInvalid(const struct LDTimestamp *timestamp);

/* Returns true if a is before b. */
LDBoolean
LDTimestamp_Before(const struct LDTimestamp *a, const struct LDTimestamp *b);

/* Returns true if a == b. */
LDBoolean
LDTimestamp_Equal(const struct LDTimestamp *a, const struct LDTimestamp *b);

/* Constructs an LDJSON object representing the timestamp in Unix milliseconds.
 * If the timestamp is known to be invalid, the marshalled value will be -1.
 * This may be unacceptable to upstream services, so validity should always
 * be checked before marshalling. */
struct LDJSON *
LDTimestamp_MarshalUnixMillis(const struct LDTimestamp *timestamp);

/* Returns a double representation of the timestamp in Unix milliseconds.
 * If the timestamp is known to be invalid, the return value will be -1. */
double
LDTimestamp_AsUnixMillis(const struct LDTimestamp *timestamp);
