#include "gtest/gtest.h"

extern "C" {
#include <string.h>

#include <launchdarkly/boolean.h>
#include <launchdarkly/logging.h>
#include "logging.h"
}

class LoggingFixture : public ::testing::Test {
};

TEST_F(LoggingFixture, LogLevelToString)
{
    ASSERT_STREQ(LDLogLevelToString(LD_LOG_FATAL), "LD_LOG_FATAL");
    ASSERT_STREQ(LDLogLevelToString(LD_LOG_CRITICAL), "LD_LOG_CRITICAL");
    ASSERT_STREQ(LDLogLevelToString(LD_LOG_ERROR), "LD_LOG_ERROR");
    ASSERT_STREQ(LDLogLevelToString(LD_LOG_WARNING), "LD_LOG_WARNING");
    ASSERT_STREQ(LDLogLevelToString(LD_LOG_INFO), "LD_LOG_INFO");
    ASSERT_STREQ(LDLogLevelToString(LD_LOG_DEBUG), "LD_LOG_DEBUG");
    ASSERT_STREQ(LDLogLevelToString(LD_LOG_TRACE), "LD_LOG_TRACE");
    ASSERT_STREQ(LDLogLevelToString(LD_LOG_TRACE), "LD_LOG_TRACE");
    ASSERT_EQ(LDLogLevelToString(static_cast<const LDLogLevel>(100)), nullptr);
}

static LDLogLevel logLevel;
static char       logBuffer[4096];

static void
bufferLogger(const LDLogLevel level, const char *const text)
{
    logLevel = level;
    memcpy(logBuffer, text, strlen(text) + 1);
}

static LDBoolean
endsWith(const char *const text, const char *const suffix)
{
    const size_t textLen   = strlen(text);
    const size_t suffixLen = strlen(suffix);

    if (textLen < suffixLen) {
        return LDBooleanFalse;
    } else {
        return (0 == strcmp(text + (textLen - suffixLen), suffix));
    }
}

TEST_F(LoggingFixture, LoggingFormatting)
{
    LDConfigureGlobalLogger(LD_LOG_TRACE, bufferLogger);

    LD_LOG(LD_LOG_FATAL, "test");
    ASSERT_EQ(logLevel, LD_LOG_FATAL);
    ASSERT_TRUE(endsWith(logBuffer, "test"));

    LD_LOG_1(LD_LOG_CRITICAL, "abc %d", 32);
    ASSERT_EQ(logLevel, LD_LOG_CRITICAL);
    ASSERT_TRUE(endsWith(logBuffer, "abc 32"));

    LD_LOG_2(LD_LOG_ERROR, "abc %d-%d", 15, 41);
    ASSERT_EQ(logLevel, LD_LOG_ERROR);
    ASSERT_TRUE(endsWith(logBuffer, "abc 15-41"));

    LD_LOG_3(LD_LOG_WARNING, "abc %d a %d %d", 19, 22, 1);
    ASSERT_EQ(logLevel, LD_LOG_WARNING);
    ASSERT_TRUE(endsWith(logBuffer, "abc 19 a 22 1"));
}

/* Basic testing to ensure this does not crash and debug tools report no errors.
 */
TEST_F(LoggingFixture, BasicLoggers)
{
    LDBasicLoggerThreadSafeInitialize();
    LDConfigureGlobalLogger(LD_LOG_INFO, LDBasicLoggerThreadSafe);
    LDi_log(LD_LOG_INFO, "test");
    LDBasicLoggerThreadSafeShutdown();
}

TEST_F(LoggingFixture, RespectsLogLevel)
{
    LDConfigureGlobalLogger(LD_LOG_INFO, bufferLogger);

    LD_LOG(LD_LOG_INFO, "a");
    ASSERT_EQ(logLevel, LD_LOG_INFO);
    ASSERT_TRUE(endsWith(logBuffer, "a"));

    LD_LOG(LD_LOG_DEBUG, "b");
    ASSERT_EQ(logLevel, LD_LOG_INFO);
    ASSERT_TRUE(endsWith(logBuffer, "a"));

    LD_LOG(LD_LOG_WARNING, "c");
    ASSERT_EQ(logLevel, LD_LOG_WARNING);
    ASSERT_TRUE(endsWith(logBuffer, "c"));
}
