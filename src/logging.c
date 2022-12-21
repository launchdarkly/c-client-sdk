#include <stdarg.h>
#include <stdio.h>

#include <launchdarkly/logging.h>

#include "assertion.h"
#include "concurrency.h"

static LDLogLevel sdkloggerlevel = LD_LOG_INFO;
static void (*sdklogger)(const LDLogLevel level, const char *const text) = NULL;
static ld_mutex_t basicLoggerLock;

const char *
LDLogLevelToString(const LDLogLevel level)
{
    switch (level) {
    case LD_LOG_FATAL:
        return "LD_LOG_FATAL";
        break;
    case LD_LOG_CRITICAL:
        return "LD_LOG_CRITICAL";
        break;
    case LD_LOG_ERROR:
        return "LD_LOG_ERROR";
        break;
    case LD_LOG_WARNING:
        return "LD_LOG_WARNING";
        break;
    case LD_LOG_INFO:
        return "LD_LOG_INFO";
        break;
    case LD_LOG_DEBUG:
        return "LD_LOG_DEBUG";
        break;
    case LD_LOG_TRACE:
        return "LD_LOG_TRACE";
        break;
    }

    return NULL;
}

void
LDBasicLogger(const LDLogLevel level, const char *const text)
{
    printf("[%s] %s\n", LDLogLevelToString(level), text);
}

void
LDBasicLoggerThreadSafeInitialize(void)
{
    LDi_mutex_nl_init(&basicLoggerLock);
}

void
LDBasicLoggerThreadSafe(const LDLogLevel level, const char *const text)
{
    if (!LDi_mutex_nl_lock(&basicLoggerLock)) {
        return;
    }

    printf("[%s] %s\n", LDLogLevelToString(level), text);

    LDi_mutex_nl_unlock(&basicLoggerLock);
}

void
LDBasicLoggerThreadSafeShutdown(void)
{
    LDi_mutex_nl_destroy(&basicLoggerLock);
}

void
LDConfigureGlobalLogger(
    const LDLogLevel level,
    void (*logger)(const LDLogLevel level, const char *const text))
{
    sdklogger      = logger;
    sdkloggerlevel = level;
}

void
LDi_log(const LDLogLevel level, const char *const format, ...)
{
    char    buffer[4096];
    va_list va;

    if (!format || !sdklogger || level > sdkloggerlevel) {
        return;
    }

    va_start(va, format);
    vsnprintf(buffer, sizeof(buffer), format, va);
    va_end(va);

    sdklogger(level, buffer);
}
