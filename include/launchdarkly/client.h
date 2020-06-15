/*!
 * @file client.h
 * @brief Public Client control and variations
 */


#pragma once

#include <launchdarkly/json.h>
#include <launchdarkly/boolean.h>
#include <launchdarkly/config.h>
#include <launchdarkly/user.h>
#include <launchdarkly/export.h>

/** @brief Opaque client object **/
struct LDClient;

/** @brief The name of the primary environment for use with
 * `LDClientGetForMobileKey` */
#define LDPrimaryEnvironmentName "default"

/** @brief Current status of the client */
typedef enum {
    LDStatusInitializing = 0,
    LDStatusInitialized,
    LDStatusFailed,
    LDStatusShuttingdown,
    LDStatusShutdown
} LDStatus;

/** @brief To use detail variations you must provide a pointer to an
 * `LDVariationDetails` struct that will be filled by the evaluation function.
 *
 * The contents of `LDVariationDetails` must be freed with
 * `LDFreeDetailContents` when they are no longer needed. */
typedef struct {
    int variationIndex;
    struct LDJSON *reason;
} LDVariationDetails;

/** @brief Get a reference to the (single, global) client. */
LD_EXPORT(struct LDClient *) LDClientGet();

/** @brief Get a reference to a secondary environment established in the
 * configuration.
 *
 * If the environment name does not exist this function
 * returns `NULL`. */
LD_EXPORT(struct LDClient *) LDClientGetForMobileKey(
    const char *const keyName);

/** @brief Initialize the client with the config and user.
 * After this call,
 *
 * the `config` and `user` must not be modified. The parameter `maxwaitmilli`
 * indicates the maximum amount of time the client will wait to be fully
 * initialized. If the timeout is hit the client will be available for
 * feature flag evaluation but the results will be fallbacks. The client
 * will continue attempting to connect to LaunchDarkly in the background.
 * If `maxwaitmilli` is set to `0` then `LDClientInit` will wait indefinitely.
 *
 * Only a single initialized client from `LDClientInit` may exist at one time.
 * To initialize another instance you must first cleanup the previous client
 * with `LDClientClose`. Should you initialize with `LDClientInit` while
 * another client exists `abort` will be called. Both `LDClientInit`, and
 * `LDClientClose` are not thread safe. */
LD_EXPORT(struct LDClient *) LDClientInit(struct LDConfig *const config,
    struct LDUser *const user, const unsigned int maxwaitmilli);

/** @brief Get JSON string containing all flags */
LD_EXPORT(char *) LDClientSaveFlags(struct LDClient *const client);

/** @brief Set flag store from JSON string */
LD_EXPORT(LDBoolean) LDClientRestoreFlags(struct LDClient *const client,
    const char *const data);

/** @brief Update the client with a new user.
 *
 * The old user is freed. This will re-fetch feature flag settings from
 * LaunchDarkly. For performance reasons, user contexts should not be
 * changed frequently. */
LD_EXPORT(void) LDClientIdentify(struct LDClient *const client,
    struct LDUser *const user);

/** @brief  Send any pending events to the server. They will normally be
 *
 * flushed after a timeout, but may also be flushed manually. This operation
 * does not block. */
LD_EXPORT(void) LDClientFlush(struct LDClient *const client);

/** @brief Returns true if the client has been initialized. */
LD_EXPORT(LDBoolean) LDClientIsInitialized(struct LDClient *const client);

/** @brief Block until initialized up to timeout, returns true if initialized */
LD_EXPORT(LDBoolean) LDClientAwaitInitialized(struct LDClient *const client,
    const unsigned int timeoutmilli);

/** @brief Returns the offline status of the client. */
LD_EXPORT(LDBoolean) LDClientIsOffline(struct LDClient *const client);

/** @brief Make the client operate in offline mode. No network traffic. */
LD_EXPORT(void) LDClientSetOffline(struct LDClient *const client);

/** @brief Return the client to online mode. */
LD_EXPORT(void) LDClientSetOnline(struct LDClient *const client);

/** @brief Enable or disable polling mode */
LD_EXPORT(void) LDClientSetBackground(struct LDClient *const client,
    const LDBoolean background);

/** @brief Close the client, free resources, and generally shut down.
 *
 * This will additionally close all secondary environments. Do not attempt to
 * manage secondary environments directly. */
LD_EXPORT(void) LDClientClose(struct LDClient *const client);

/** @brief Add handler for when client status changes */
LD_EXPORT(void) LDSetClientStatusCallback(void (callback)(int status));

/** @brief Record a custom event. */
LD_EXPORT(void) LDClientTrack(struct LDClient *const client,
    const char *const name);

/** @brief Record a custom event and include custom data. */
LD_EXPORT(void) LDClientTrackData(struct LDClient *const client,
    const char *const name, struct LDJSON *const data);

/** @brief Record a custom event and include custom data / a metric. */
LD_EXPORT(void) LDClientTrackMetric(struct LDClient *const client,
    const char *const name, struct LDJSON *const data, const double metric);

/** @brief  Returns an object of all flags. This must be freed with
 * `LDJSONFree`. */
LD_EXPORT(struct LDJSON *) LDAllFlags(struct LDClient *const client);

/** @brief Evaluate Bool flag */
LD_EXPORT(LDBoolean) LDBoolVariation(struct LDClient *const client,
    const char *const featureKey, const LDBoolean fallback);

/** @brief Evaluate Int flag
 *
 * If the flag value is actually a float the result is truncated. */
LD_EXPORT(int) LDIntVariation(struct LDClient *const client,
    const char *const featureKey, const int fallback);

/** @brief Evaluate Double flag */
LD_EXPORT(double) LDDoubleVariation(struct LDClient *const client,
    const char *const featureKey, const double fallback);

/** @brief Evaluate String flag */
LD_EXPORT(char *) LDStringVariationAlloc(struct LDClient *const client,
    const char *const featureKey, const char *const fallback);

/** @brief Evaluate String flag into fixed buffer */
LD_EXPORT(char *) LDStringVariation(struct LDClient *const client,
    const char *const featureKey, const char *const fallback,
    char *const resultBuffer, const size_t resultBufferSize);

/** @brief Evaluate JSON flag */
LD_EXPORT(struct LDJSON *) LDJSONVariation(struct LDClient *const client,
    const char *const featureKey, const struct LDJSON *const fallback);

/** @brief Evaluate Bool flag with details */
LD_EXPORT(LDBoolean) LDBoolVariationDetail(struct LDClient *const client,
    const char *const featureKey, const LDBoolean fallback,
    LDVariationDetails *const details);

/** @brief Evaluate Int flag with details
 *
 * If the flag value is actually a float the result is truncated. */
LD_EXPORT(int) LDIntVariationDetail(struct LDClient *const client,
    const char *const featureKey, const int fallback,
    LDVariationDetails *const details);

/** @brief Evaluate Double flag with details */
LD_EXPORT(double) LDDoubleVariationDetail(struct LDClient *const client,
    const char *const featureKey, const double fallback,
    LDVariationDetails *const details);

/** @brief Evaluate String flag with details */
LD_EXPORT(char *) LDStringVariationAllocDetail(struct LDClient *const client,
    const char *const featureKey, const char *const fallback,
    LDVariationDetails *const details);

/** @brief Evaluate String flag into fixed buffer with details */
LD_EXPORT(char *) LDStringVariationDetail(struct LDClient *const client,
    const char *const featureKey, const char *const fallback,
    char *const resultBuffer, const size_t resultBufferSize,
    LDVariationDetails *const details);

/** @brief Evaluate JSON flag with details */
LD_EXPORT(struct LDJSON *) LDJSONVariationDetail(
    struct LDClient *const client, const char *const key,
    const struct LDJSON *const fallback, LDVariationDetails *const details);

/** @brief Clear any memory associated with `LDVariationDetails`  */
LD_EXPORT(void) LDFreeDetailContents(LDVariationDetails details);

/** @brief Feature flag listener callback type. Callbacks are not reentrant
 * safe.
 *
 * Status 0 for new or updated, 1 for deleted. */
typedef void (*LDlistenerfn)(const char *const flagKey, const int status);

/** @brief Register a callback for when a flag is updated. */
LD_EXPORT(LDBoolean) LDClientRegisterFeatureFlagListener(
  struct LDClient *const client, const char *const flagKey,
  LDlistenerfn listener);

/** @brief Unregister a callback registered with
* `LDClientRegisterFeatureFlagListener` */
LD_EXPORT(void) LDClientUnregisterFeatureFlagListener(
    struct LDClient *const client, const char *const flagKey,
    LDlistenerfn listener);
