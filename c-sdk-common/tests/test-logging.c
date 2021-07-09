#include <string.h>

#include <launchdarkly/boolean.h>
#include <launchdarkly/logging.h>

#include "assertion.h"
#include "logging.h"

static void
testLogLevelToString(void)
{
    LD_ASSERT(strcmp(LDLogLevelToString(LD_LOG_FATAL),
        "LD_LOG_FATAL") == 0);
    LD_ASSERT(strcmp(LDLogLevelToString(LD_LOG_CRITICAL),
        "LD_LOG_CRITICAL") == 0);
    LD_ASSERT(strcmp(LDLogLevelToString(LD_LOG_ERROR),
        "LD_LOG_ERROR") == 0);
    LD_ASSERT(strcmp(LDLogLevelToString(LD_LOG_WARNING),
        "LD_LOG_WARNING") == 0);
    LD_ASSERT(strcmp(LDLogLevelToString(LD_LOG_INFO),
        "LD_LOG_INFO") == 0);
    LD_ASSERT(strcmp(LDLogLevelToString(LD_LOG_DEBUG),
        "LD_LOG_DEBUG") == 0);
    LD_ASSERT(strcmp(LDLogLevelToString(LD_LOG_TRACE),
        "LD_LOG_TRACE") == 0);
    LD_ASSERT(strcmp(LDLogLevelToString(LD_LOG_TRACE),
        "LD_LOG_TRACE") == 0);
    LD_ASSERT(LDLogLevelToString(100) == NULL);
}

static LDLogLevel logLevel;
static char logBuffer[4096];

static void
bufferLogger(const LDLogLevel level, const char *const text)
{
    logLevel = level;
    memcpy(logBuffer, text, strlen(text) + 1);
}

static LDBoolean
endsWith(const char *const text, const char *const suffix)
{
    const size_t textLen = strlen(text);
    const size_t suffixLen = strlen(suffix);

    if (textLen < suffixLen) {
        return LDBooleanFalse;
    } else {
        return (0 == strcmp(text + (textLen - suffixLen), suffix));
    }
}

static void
testLoggingFormatting(void)
{
    LDConfigureGlobalLogger(LD_LOG_TRACE, bufferLogger);

    LD_LOG(LD_LOG_FATAL, "test");
    LD_ASSERT(logLevel == LD_LOG_FATAL);
    LD_ASSERT(endsWith(logBuffer, "test"));

    LD_LOG_1(LD_LOG_CRITICAL, "abc %d", 32);
    LD_ASSERT(logLevel == LD_LOG_CRITICAL);
    LD_ASSERT(endsWith(logBuffer, "abc 32"));

    LD_LOG_2(LD_LOG_ERROR, "abc %d-%d", 15, 41);
    LD_ASSERT(logLevel == LD_LOG_ERROR);
    LD_ASSERT(endsWith(logBuffer, "abc 15-41"));

    LD_LOG_3(LD_LOG_WARNING, "abc %d a %d %d", 19, 22, 1);
    LD_ASSERT(logLevel == LD_LOG_WARNING);
    LD_ASSERT(endsWith(logBuffer, "abc 19 a 22 1"));
}

/* Basic testing to ensure this does not crash and debug tools report no errors.
*/
static void
testBasicLoggers(void)
{
    LDBasicLoggerThreadSafeInitialize();
    LDConfigureGlobalLogger(LD_LOG_INFO, LDBasicLoggerThreadSafe);
    LDi_log(LD_LOG_INFO, "test");
    LDBasicLoggerThreadSafeShutdown();
}

static void
testRespectsLogLevel(void)
{
    LDConfigureGlobalLogger(LD_LOG_INFO, bufferLogger);

    LD_LOG(LD_LOG_INFO, "a");
    LD_ASSERT(logLevel == LD_LOG_INFO);
    LD_ASSERT(endsWith(logBuffer, "a"));

    LD_LOG(LD_LOG_DEBUG, "b");
    LD_ASSERT(logLevel == LD_LOG_INFO);
    LD_ASSERT(endsWith(logBuffer, "a"));

    LD_LOG(LD_LOG_WARNING, "c");
    LD_ASSERT(logLevel == LD_LOG_WARNING);
    LD_ASSERT(endsWith(logBuffer, "c"));
}

int
main(void)
{
    testLogLevelToString();
    testLoggingFormatting();
    testBasicLoggers();
    testRespectsLogLevel();

    return 0;
}
