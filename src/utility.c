#include <string.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
#define _CRT_RAND_S
#endif
#include <stdlib.h>

#include <launchdarkly/json.h>
#include <launchdarkly/memory.h>

#include "assertion.h"
#include "concurrency.h"
#include "utility.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <unistd.h>
#endif

int
LDi_strncasecmp(const char *const s1, const char *const s2, const size_t n)
{
#ifdef _WIN32
    return _strnicmp(s1, s2, n);
#else
    return strncasecmp(s1, s2, n);
#endif
}

LDBoolean
LDi_random(unsigned int *const result)
{
#ifdef _WIN32
    LD_ASSERT(result);
    errno_t status;
    status = rand_s(result);
    LD_ASSERT(status == 0);
    return status == 0;
#else
    static unsigned int state;
    static LDBoolean    init = LDBooleanFalse;
    static ld_mutex_t   lock = PTHREAD_MUTEX_INITIALIZER;

    LD_ASSERT(result);

    LDi_mutex_lock(&lock);

    if (!init) {
        state = time(NULL);
        init  = LDBooleanTrue;
    }

    *result = rand_r(&state);

    LDi_mutex_unlock(&lock);

    return LDBooleanTrue;
#endif
}

LDBoolean
LDi_sleepMilliseconds(const unsigned long milliseconds)
{
#ifdef _WIN32
    Sleep(milliseconds);

    return LDBooleanTrue;
#else
    int        status;
    useconds_t usec;

    if (milliseconds >= 1000) {
        usec = 999999;

        LD_LOG(
            LD_LOG_WARNING, "LDi_sleepMilliseconds capping usleep at 999999");
    } else {
        usec = milliseconds * 1000;
    }

    if ((status = usleep(usec)) != 0) {
        if (errno == EINTR) {
            LD_LOG(
                LD_LOG_WARNING,
                "LDi_sleepMilliseconds usleep got "
                "EINTR skipping sleep");

            return LDBooleanFalse;
        } else {
            LD_LOG_1(
                LD_LOG_CRITICAL, "usleep failed with: %s", strerror(errno));

            LD_ASSERT(LDBooleanFalse);

            return LDBooleanFalse;
        }
    }

    return LDBooleanTrue;
#endif
}

#ifndef _WIN32
static double
LDi_timeSpecToMilliseconds(const struct timespec ts)
{
    return floor(((double)ts.tv_sec * 1000.0) + ((double)ts.tv_nsec / 1000000.0));
}

LDBoolean
LDi_clockGetTime(struct timespec *const ts, ld_clock_t clockid)
{
#ifdef __APPLE__
    kern_return_t   status;
    clock_serv_t    clock_serve;
    mach_timespec_t mach_timespec;

    status = host_get_clock_service(mach_host_self(), clockid, &clock_serve);

    if (status != KERN_SUCCESS) {
        return LDBooleanFalse;
    }

    status = clock_get_time(clock_serve, &mach_timespec);
    mach_port_deallocate(mach_task_self(), clock_serve);

    if (status != KERN_SUCCESS) {
        return LDBooleanFalse;
    }

    ts->tv_sec  = mach_timespec.tv_sec;
    ts->tv_nsec = mach_timespec.tv_nsec;
#else
    if (clock_gettime(clockid, ts) != 0) {
        return LDBooleanFalse;
    }
#endif

    return LDBooleanTrue;
}
#endif

LDBoolean
LDi_getMonotonicMilliseconds(double *const resultMilliseconds)
{
#ifdef _WIN32
    *resultMilliseconds = (double)GetTickCount64();

    return LDBooleanTrue;
#else
    struct timespec ts;
    if (LDi_clockGetTime(&ts, LD_CLOCK_MONOTONIC)) {
        *resultMilliseconds = LDi_timeSpecToMilliseconds(ts);

        return LDBooleanTrue;
    } else {
        LD_ASSERT(LDBooleanFalse);

        return LDBooleanFalse;
    }
#endif
}

LDBoolean
LDi_getUnixMilliseconds(double *const resultMilliseconds)
{
#ifdef _WIN32
    *resultMilliseconds = floor((double)time(NULL) * 1000.0);

    return LDBooleanTrue;
#else
    struct timespec ts;
    if (LDi_clockGetTime(&ts, LD_CLOCK_REALTIME)) {
        *resultMilliseconds = LDi_timeSpecToMilliseconds(ts);

        return LDBooleanTrue;
    } else {
        LD_ASSERT(LDBooleanFalse);

        return LDBooleanFalse;
    }
#endif
}

LDBoolean
LDSetString(char **const target, const char *const value)
{
    if (value) {
        char *tmp;

        if ((tmp = LDStrDup(value))) {
            LDFree(*target);

            *target = tmp;

            return LDBooleanTrue;
        } else {
            return LDBooleanFalse;
        }
    } else {
        LDFree(*target);

        *target = NULL;

        return LDBooleanTrue;
    }
}

double
LDi_normalize(
    const double n,
    const double nmin,
    const double nmax,
    const double omin,
    const double omax)
{
    return (n - nmin) * (omax - omin) / (nmax - nmin) + omin;
}

LDBoolean
LDi_randomHex(char *const buffer, const size_t bufferSize)
{
    size_t            i;
    const char *const alphabet = "0123456789ABCDEF";

    for (i = 0; i < bufferSize; i++) {
        unsigned int rng = 0;
        if (LDi_random(&rng)) {
            buffer[i] = alphabet[rng % 16];
        } else {
            return LDBooleanFalse;
        }
    }

    return LDBooleanTrue;
}

LDBoolean
LDi_UUIDv4(char *const buffer)
{
    if (!LDi_randomHex(buffer, LD_UUID_SIZE)) {
        return LDBooleanFalse;
    }

    buffer[8]  = '-';
    buffer[13] = '-';
    buffer[18] = '-';
    buffer[23] = '-';

    return LDBooleanTrue;
}
