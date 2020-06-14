/*!
 * @file config.h
 * @brief Public API Interface for Configuration
 */

#pragma once

#include <launchdarkly/boolean.h>
#include <launchdarkly/json.h>
#include <launchdarkly/export.h>

/** @brief Opaque configuration object */
struct LDConfig;

/** @brief Creates a new default configuration. `mobileKey` is required.
 *
 * The configuration object is intended to be modified until it is passed to
 * `LDClientInit`, at which point it should no longer be modified. */
LD_EXPORT(struct LDConfig *) LDConfigNew(const char *const mobileKey);

/** @brief Marks all user attributes private. */
LD_EXPORT(void) LDConfigSetAllAttributesPrivate(struct LDConfig *const config,
    const LDBoolean allPrivate);

/** @brief Sets the interval in milliseconds between polls for flag updates
 * when your app is in the background. */
LD_EXPORT(void) LDConfigSetBackgroundPollingIntervalMillis(
    struct LDConfig *const config, const int millis);

/** @brief Sets the interval in milliseconds between polls for flag updates
 * when your app is in the background. */
LD_EXPORT(LDBoolean) LDConfigSetAppURI(struct LDConfig *const config,
    const char *const uri);

/** @brief Sets the timeout in milliseconds when connecting to LaunchDarkly. */
LD_EXPORT(void) LDConfigSetConnectionTimeoutMillies(
    struct LDConfig *const config, const int millis);

/** @brief Enable or disable background updating */
LD_EXPORT(void) LDConfigSetDisableBackgroundUpdating(
    struct LDConfig *const config, const LDBoolean disable);

/** @brief Sets the max number of events to queue before sending them to
 * LaunchDarkly. */
LD_EXPORT(void) LDConfigSetEventsCapacity(struct LDConfig *const config,
    const int capacity);

/** @brief Sets the maximum amount of time in milliseconds to wait in between
 * sending analytics events to LaunchDarkly. */
LD_EXPORT(void) LDConfigSetEventsFlushIntervalMillis(
    struct LDConfig *const config, const int millis);

/** @brief Set the events uri for sending analytics to LaunchDarkly. You
 * probably don't need to set this unless instructed by LaunchDarkly. */
LD_EXPORT(LDBoolean) LDConfigSetEventsURI(struct LDConfig *const config,
    const char *const uri);

/** @brief Sets the key for authenticating with LaunchDarkly. This is required
 * unless you're using the client in offline mode. */
LD_EXPORT(LDBoolean) LDConfigSetMobileKey(struct LDConfig *const config,
    const char *const key);

/** @brief Configures the client for offline mode. In offline mode, no
 * external network connections are made. */
LD_EXPORT(void) LDConfigSetOffline(struct LDConfig *const config,
    const LDBoolean offline);

/** @brief Enables or disables real-time streaming flag updates.
 *
 * Default: `true`. When set to `false`, an efficient caching polling
 * mechanism is used. We do not recommend disabling `streaming` unless you
 * have been instructed to do so by LaunchDarkly support. */
LD_EXPORT(void) LDConfigSetStreaming(struct LDConfig *const config,
    const LDBoolean streaming);

/** @brief Only relevant when `streaming` is disabled (set to `false`). Sets
 * the interval between feature flag updates. */
LD_EXPORT(void) LDConfigSetPollingIntervalMillis(struct LDConfig *const config,
    const int millis);

/** @brief Set the stream uri for connecting to the flag update stream. You
 * probably don't need to set this unless instructed by LaunchDarkly. */
LD_EXPORT(LDBoolean) LDConfigSetStreamURI(struct LDConfig *const config,
    const char *const uri);

/** @brief Set the proxy server used for connecting to LaunchDarkly.
 *
 * By default no proxy is used. The URI string should be of the form
 * `socks5://127.0.0.1:9050`. You may read more about how this SDK handles
 * proxy servers by reading the [libcurl](https://curl.haxx.se) documentation
 * on the subject [here](https://ec.haxx.se/libcurl-proxies.html). */
LD_EXPORT(LDBoolean) LDConfigSetProxyURI(struct LDConfig *const config,
    const char *const uri);

/** @brief Set whether to verify the authenticity of the peer's certificate on
 * network requests.
 *
 * By default peer verification is enabled. You may read more about what this
 * means by reading the [libcurl](https://curl.haxx.se) documentation on the
 * subject [here](https://curl.haxx.se/libcurl/c/CURLOPT_SSL_VERIFYPEER.html).
 */
LD_EXPORT(void) LDConfigSetVerifyPeer(struct LDConfig *const config,
    const LDBoolean enabled);

/** @brief Determines whether the `REPORT` or `GET` verb is used for calls to
 * LaunchDarkly. Do not use unless advised by LaunchDarkly. */
LD_EXPORT(void) LDConfigSetUseReport(struct LDConfig *const config,
    const LDBoolean report);

/** @brief Decide whether the client should fetch feature flag evaluation
 * explanations from LaunchDarkly. */
LD_EXPORT(void) LDConfigSetUseEvaluationReasons(struct LDConfig *const config,
    const LDBoolean reasons);

/** @brief Private attribute list which will not be recorded for all users. */
LD_EXPORT(void) LDConfigSetPrivateAttributes(struct LDConfig *const config,
    struct LDJSON *attributes);

/** @brief Add another mobile key to the list of secondary environments.
 *
 * Both `name`, and `key` must be unique. You may not add the existing primary
 * environment (the one you used to initialize `LDConfig`). The `name` of the
 * key can later be used in conjunction with `LDClientGetForMobileKey`. This
 * function returns false on failure. */
LD_EXPORT(LDBoolean) LDConfigAddSecondaryMobileKey(
    struct LDConfig *const config, const char *const name,
    const char *const key);

/** @brief Set the path to the SSL certificate bundle used for peer
 * authentication.
 *
 * This API is ineffective if LDConfigSetVerifyPeer is set to false. See
 * [CURLOPT_CAINFO](https://curl.haxx.se/libcurl/c/CURLOPT_CAINFO.html) for
 * more information. */
LD_EXPORT(LDBoolean) LDConfigSetSSLCertificateAuthority(struct LDConfig *config,
    const char *certFile);

/** @brief Determines if an entire user object, or only the user key should be
 * included in events. Defaults to false. */
LD_EXPORT(void) LDConfigSetInlineUsersInEvents(struct LDConfig *const config,
    const LDBoolean inlineUsers);

/** @brief Free an existing `LDConfig` instance.
 *
 * You will likely never use this routine as ownership is transferred to
 * `LDClient` on initialization. */
LD_EXPORT(void) LDConfigFree(struct LDConfig *const config);
