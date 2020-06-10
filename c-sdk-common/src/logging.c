#include <stdio.h>
#include <stdarg.h>

#include <launchdarkly/logging.h>

#include "assertion.h"

static LDLogLevel sdkloggerlevel = LD_LOG_INFO;
static void (*sdklogger)(const LDLogLevel level, const char *const text) = NULL;

const char *
LDLogLevelToString(const LDLogLevel level)
{
    switch (level) {
        case LD_LOG_FATAL:    return "LD_LOG_FATAL";    break;
        case LD_LOG_CRITICAL: return "LD_LOG_CRITICAL"; break;
        case LD_LOG_ERROR:    return "LD_LOG_ERROR";    break;
        case LD_LOG_WARNING:  return "LD_LOG_WARNING";  break;
        case LD_LOG_INFO:     return "LD_LOG_INFO";     break;
        case LD_LOG_DEBUG:    return "LD_LOG_DEBUG";    break;
        case LD_LOG_TRACE:    return "LD_LOG_TRACE";    break;
    }

    return NULL;
}

void
LDBasicLogger(const LDLogLevel level, const char *const text)
{
    printf("[%s] %s\n", LDLogLevelToString(level), text);
}

void
LDConfigureGlobalLogger(const LDLogLevel level,
    void (*logger)(const LDLogLevel level, const char *const text))
{
    sdklogger = logger;
    sdkloggerlevel = level;
}

void
LDi_log(const LDLogLevel level, const char *const format, ...)
{
    char buffer[4096];
    va_list va;

    LD_ASSERT(format);

    if (!sdklogger || level > sdkloggerlevel) {
        return;
    }

    va_start(va, format);
    vsnprintf(buffer, sizeof(buffer), format, va);
    va_end(va);

    sdklogger(level, buffer);
}
