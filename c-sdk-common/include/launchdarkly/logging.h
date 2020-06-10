/*!
 * @file logging.h
 * @brief Public API Interface for Logging.
 */

#pragma once

#include <launchdarkly/export.h>

/** @brief The log levels compatible with the logging interface */
typedef enum {
    LD_LOG_FATAL = 0,
    LD_LOG_CRITICAL,
    LD_LOG_ERROR,
    LD_LOG_WARNING,
    LD_LOG_INFO,
    LD_LOG_DEBUG,
    LD_LOG_TRACE
} LDLogLevel;

/** @brief Internal: Used for the non macro portion */
LD_EXPORT(void) LDi_log(const LDLogLevel level, const char *const format, ...);

/** @brief A provided logger that can be used as a convenient default */
LD_EXPORT(void) LDBasicLogger(const LDLogLevel level, const char *const text);

/**
 * @brief Set the logger, and the log level to use. This routine should only be
 * used before any other LD routine. After any other routine has been used
 * setting the logger is no longer a safe operation and may result in undefined
 * behavior manifesting itself.
 * @param[in] level The verbosity of logs to send to logger.
 * @param[in] logger The new function to use for all future logging.
 * @return Void.
 */
LD_EXPORT(void) LDConfigureGlobalLogger(const LDLogLevel level,
    void (*logger)(const LDLogLevel level, const char *const text));

/** @brief A macro interface that allows convenient logging of line numbers */
#define LD_LOG(level, text) \
    LDi_log(level, "[%s, %d] %s", __FILE__, __LINE__, text)

/**
 * @brief Convert a verbosity level Enum value to an equivalent static string.
 * This is intended as a convenience operation for building other loggers.
 * @param[in] level The log level to convert.
 * @return A static string on success, `NULL` on failure.
 */
LD_EXPORT(const char *) LDLogLevelToString(const LDLogLevel level);
