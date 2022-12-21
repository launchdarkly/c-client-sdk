#pragma once
#include <launchdarkly/boolean.h>
#include <time.h>

/* LDTimer allows for measurement of an elapsed duration between
 * two points in time, using a monotonic clock. */
struct LDTimer {
    double ms;
};

/* LDTimestamp allows for retrieving and comparing wall-clock timestamps.
 * Additionally, an LDTimestamp can be marshalled directly to LDJSON.
 * To construct a timestamp, see the various LDTimestamp_Init methods. */
struct LDTimestamp {
    double ms;
};

/* Reset an LDTimer. Once an LDTimer has been reset, LDTimer_Elapsed may be called
 * to retrieve the elapsed milliseconds between the call to LDTimer_Elapsed and LDTimer_Reset.
 * Returns false if obtaining a monotonic timestamp failed.
 * Must not be NULL. */
LDBoolean
LDTimer_Reset(struct LDTimer *timer);

/* Retrieve the elapsed time between a previous call to LDTimer_Reset and now, in milliseconds.
 * Returns false if obtaining a monotonic timestamp failed.
 * Timer and out_elapsedMs must not be NULL. */
LDBoolean
LDTimer_Elapsed(const struct LDTimer *timer, double *out_elapsedMs);

/* Initializes a timestamp to an "empty" state. This state can be checked using
 * LDTimestamp_IsEmpty/IsSet. Must not be NULL. */
void
LDTimestamp_InitEmpty(struct LDTimestamp *timestamp);

/* Initialize a timestamp with the current wall time.
 * Obtaining a timestamp may fail, which can be detected immediately by a false return value,
 * or later using LDTimestamp_IsSet. */
LDBoolean
LDTimestamp_InitNow(struct LDTimestamp *timestamp);

/* Initialize a timestamp from a time_t value.
 * Behavior is defined only if the value is >= 0. */
void
LDTimestamp_InitUnixSeconds(struct LDTimestamp *timestamp, time_t unixSeconds);

/* Initialize a timestamp from a double representing Unix milliseconds.
 * Behavior is defined only if the value is >= 0. */
void
LDTimestamp_InitUnixMillis(struct LDTimestamp *timestamp, double unixMilliseconds);

/* Returns true if a < b. Both timestamps must be present and not-NULL. */
LDBoolean
LDTimestamp_Before(const struct LDTimestamp *a, const struct LDTimestamp *b);

/* Returns true if a > b. Both timestamps must be present and not-NULL.*/
LDBoolean
LDTimestamp_After(const struct LDTimestamp *a, const struct LDTimestamp *b);

/* Returns true if a == b. Both timestamps must be present and not-NULL. */
LDBoolean
LDTimestamp_Equal(const struct LDTimestamp *a, const struct LDTimestamp *b);

/* Returns true if the timestamp was initialized to empty. Must not be NULL. */
LDBoolean
LDTimestamp_IsEmpty(const struct LDTimestamp *timestamp);

/* Returns true if the timestamp was initialized with a valid time. Must not be NULL. */
LDBoolean
LDTimestamp_IsSet(const struct LDTimestamp *timestamp);

/* Constructs an LDJSON text object representing the timestamp in Unix milliseconds,
 * or an LDJSON null if the timestamp is empty. Must not be NULL. */
struct LDJSON *
LDTimestamp_MarshalUnixMillis(const struct LDTimestamp *timestamp);

/* Returns a double representation of the timestamp in Unix milliseconds.
 * Behavior is defined only if the timestamp is not empty. Must not be NULL. */
double
LDTimestamp_AsUnixMillis(const struct LDTimestamp *timestamp);
