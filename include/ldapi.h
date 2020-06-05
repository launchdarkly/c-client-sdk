/*!
 * @file ldapi.h
 * @brief Public API. Include this for every public operation.
 */

#ifndef C_CLIENT_LDIAPI_H
#define C_CLIENT_LDIAPI_H

/** @brief The current SDK version string. This value adheres to semantic
 * versioning and is included in the HTTP user agent sent to LaunchDarkly. */
#define LD_SDK_VERSION "1.7.6"

#ifdef __cplusplus
#include <string>

extern "C" {
#endif

#include <launchdarkly/export.h>
#include <launchdarkly/logging.h>
#include <launchdarkly/memory.h>
#include <launchdarkly/json.h>
#include <launchdarkly/config.h>
#include <launchdarkly/user.h>
#include <launchdarkly/client.h>

#include "config.h"
#include "concurrency.h"
#include "logging.h"

#include "uthash.h"

/** @brief Should write the `data` using the associated `context` to `name`.
 * Returns `true` for success. */
typedef bool (*LD_store_stringwriter)(void *const context,
    const char *const name, const char *const data);

/** @brief Read a string from the `name` associated with the `context`.
 * Returns `NULL` on failure.
 *
 * Memory returned from the reader must come from `LDAlloc`. */
typedef char *(*LD_store_stringreader)(void *const context,
    const char *const name);

/** @brief Sets the storage functions to be used.
 *
 * Reader and writer may optionally be `NULL`. The provided opaque context
 * is passed to each open call. */
LD_EXPORT(void) LD_store_setfns(void *const context, LD_store_stringwriter,
    LD_store_stringreader);

/** @brief Predefined `ld_store_stringwriter` for files. */
LD_EXPORT(LDBoolean) LD_store_filewrite(void *const context,
    const char *const name, const char *const data);

/** @brief Predefined `ld_store_stringreader` for files */
LD_EXPORT(char *) LD_store_fileread(void *const context,
    const char *const name);

#ifdef __cplusplus
}
#endif

#endif /* C_CLIENT_LDIAPI_H */
