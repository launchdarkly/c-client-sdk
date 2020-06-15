/*!
 * @file ldinternal.h
 * @brief Internal Miscellaneous Implementation Details
 */

#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __APPLE__
    #include <mach/clock.h>
    #include <mach/mach.h>
#endif

#ifndef __WIN32
    #include <time.h>
#endif

#include <launchdarkly/json.h>

/* **** LDUtility **** */

#define LD_UUID_SIZE 36

bool LDi_sleepMilliseconds(const unsigned long milliseconds);
bool LDi_getMonotonicMilliseconds(double *const resultMilliseconds);
bool LDi_getUnixMilliseconds(double *const resultMilliseconds);
bool LDi_randomHex(char *const buffer, const size_t bufferSize);
#define LD_UUID_SIZE 36
bool LDi_UUIDv4(char *const buffer);

#ifdef _WIN32
    #define LD_RAND_MAX UINT_MAX
#else
    #define LD_RAND_MAX RAND_MAX
#endif

#ifndef _WIN32
    #ifdef __APPLE__
        #define ld_clock_t clock_id_t
        #define LD_CLOCK_MONOTONIC SYSTEM_CLOCK
        #define LD_CLOCK_REALTIME CALENDAR_CLOCK
    #else
        #define ld_clock_t clockid_t
        #define LD_CLOCK_MONOTONIC CLOCK_MONOTONIC
        #define LD_CLOCK_REALTIME CLOCK_REALTIME
    #endif

    bool LDi_clockGetTime(struct timespec *const ts, ld_clock_t clockid);
#endif

/* do not use in cryptographic / security focused contexts */
bool LDi_random(unsigned int *const result);

bool LDSetString(char **const target, const char *const value);

double LDi_normalize(const double n, const double nmin, const double nmax,
    const double omin, const double omax);

bool LDi_notNull(const struct LDJSON *const json);
bool LDi_isDeleted(const struct LDJSON *const feature);
bool LDi_textInArray(const struct LDJSON *const array, const char *const text);
int LDi_strncasecmp(const char *const s1, const char *const s2, const size_t n);

/* windows does not have strptime */
#ifdef _WIN32
    const char *strptime (const char *buf, const char *fmt, struct tm *tm);
#endif
