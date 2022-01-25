/*!
 * @file api.h
 * @brief Public API. Include this for every public operation.
 */

#ifndef C_CLIENT_LDIAPI_H
#define C_CLIENT_LDIAPI_H

/** @brief The current SDK version string. This value adheres to semantic
 * versioning and is included in the HTTP user agent sent to LaunchDarkly. */
#define LD_SDK_VERSION "2.4.1"

#ifdef __cplusplus
#include <string>

extern "C" {
#endif

#include <launchdarkly/client.h>
#include <launchdarkly/config.h>
#include <launchdarkly/export.h>
#include <launchdarkly/json.h>
#include <launchdarkly/logging.h>
#include <launchdarkly/memory.h>
#include <launchdarkly/user.h>

#ifdef __cplusplus
}
#endif

#endif /* C_CLIENT_LDIAPI_H */
