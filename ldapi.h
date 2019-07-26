/*!
 * @file ldapi.h
 * @brief Public API. Include this for every public operation.
 */

#ifndef C_CLIENT_LDIAPI_H
#define C_CLIENT_LDIAPI_H

/** @brief The current SDK version string. This value adheres to semantic
 * versioning and is included in the HTTP user agent sent to LaunchDarkly. */
#define LD_SDK_VERSION "1.5.0"

/** @brief Used to ensure only intended symbols are exported in the binaries */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
    #define LD_EXPORT(x) x
#else
    #ifdef _WIN32
        #define LD_EXPORT(x) __declspec(dllexport) x
    #else
        #define LD_EXPORT(x) __attribute__((visibility("default"))) x
    #endif
#endif

#ifdef __cplusplus
#include <string>

extern "C" {
#endif

#include <stdbool.h>

#include "uthash.h"

/** @brief The name of the primary environment for use with
 * `LDClientGetForMobileKey` */
#define LDPrimaryEnvironmentName "default"

/** @brief The log levels compatible with the logging interface */
enum ld_log_level {
    LD_LOG_FATAL = 0,
    LD_LOG_CRITICAL,
    LD_LOG_ERROR,
    LD_LOG_WARNING,
    LD_LOG_INFO,
    LD_LOG_DEBUG,
    LD_LOG_TRACE
};

/** @brief Current status of the client */
typedef enum {
    LDStatusInitializing = 0,
    LDStatusInitialized,
    LDStatusFailed,
    LDStatusShuttingdown,
    LDStatusShutdown
} LDStatus;

/** @brief JSON equivalent types used by `LDNode`. */
typedef enum {
    LDNodeNone = 0,
    LDNodeString,
    LDNodeNumber,
    LDNodeBool,
    LDNodeHash,
    LDNodeArray,
} LDNodeType;

/** @brief A Node node will have a type, one of string, number, bool, hash,
 * or array. The corresponding union field, s, n, b, h, or a will be set. */
typedef struct LDNode_i {
    union {
        /** @brief Key when object. */
        char *key;
        /** @brief Key when array. */
        unsigned int idx;
    };
    /** @brief Node type. */
    LDNodeType type;
    /** @brief Node value. */
    union {
        bool b;
        char *s;
        double n;
        struct LDNode_i *h;
        struct LDNode_i *a;
    };
    /** @brief Hash handle used for hash tables. */
    UT_hash_handle hh;
    /** @brief Internal version tracking. */
    int version;
    /** @brief Internal variation tracking. */
    int variation;
    /** @brief Internal version tracking. */
    int flagversion;
    /** @brief Internal track timing. */
    double track;
    /** @brief Internal evaluation reason. */
    struct LDNode_i* reason;
#ifdef __cplusplus
    struct LDNode_i *lookup(const std::string &key);
    struct LDNode_i *index(unsigned int idx);
#endif
} LDNode;

/** @brief To use detail variations you must provide a pointer to an
 * `LDVariationDetails` struct that will be filled by the evaluation function.
 *
 * The contents of `LDVariationDetails` must be freed with
 * `LDFreeDetailContents` when they are no longer needed. */
typedef struct {
    int variationIndex;
    struct LDNode_i* reason;
} LDVariationDetails;

/** @brief Opaque configuration object */
typedef struct LDConfig_i LDConfig;

/** @brief Opaque user object **/
typedef struct LDUser_i LDUser;

/** @brief Opaque client object **/
struct LDClient_i;

/** @brief Creates a new default configuration. `mobileKey` is required.
 *
 * The configuration object is intended to be modified until it is passed to
 * `LDClientInit`, at which point it should no longer be modified. */
LD_EXPORT(LDConfig *) LDConfigNew(const char *const mobileKey);

/** @brief Marks all user attributes private. */
LD_EXPORT(void) LDConfigSetAllAttributesPrivate(LDConfig *const config,
    const bool allprivate);

/** @brief Sets the interval in milliseconds between polls for flag updates
 * when your app is in the background. */
LD_EXPORT(void) LDConfigSetBackgroundPollingIntervalMillis(
    LDConfig *const config, const int millis);

/** @brief Sets the interval in milliseconds between polls for flag updates
 * when your app is in the background. */
LD_EXPORT(void) LDConfigSetAppURI(LDConfig *const config,
    const char *const uri);

/** @brief Sets the timeout in milliseconds when connecting to LaunchDarkly. */
LD_EXPORT(void) LDConfigSetConnectionTimeoutMillies(LDConfig *const config,
    const int millis);

/** @brief Enable or disable background updating */
LD_EXPORT(void) LDConfigSetDisableBackgroundUpdating(LDConfig *const config,
    const bool disable);

/** @brief Sets the max number of events to queue before sending them to
 * LaunchDarkly. */
LD_EXPORT(void) LDConfigSetEventsCapacity(LDConfig *const config,
    const int capacity);

/** @brief Sets the maximum amount of time in milliseconds to wait in between
 * sending analytics events to LaunchDarkly. */
LD_EXPORT(void) LDConfigSetEventsFlushIntervalMillis(LDConfig *const config,
    const int millis);

/** @brief Set the events uri for sending analytics to LaunchDarkly. You
 * probably don't need to set this unless instructed by LaunchDarkly. */
LD_EXPORT(void) LDConfigSetEventsURI(LDConfig *const config,
    const char *const uri);

/** @brief Sets the key for authenticating with LaunchDarkly. This is required
 * unless you're using the client in offline mode. */
LD_EXPORT(void) LDConfigSetMobileKey(LDConfig *const config,
    const char *const key);

/** @brief Configures the client for offline mode. In offline mode, no
 * external network connections are made. */
LD_EXPORT(void) LDConfigSetOffline(LDConfig *const config, const bool offline);

/** @brief Enables or disables real-time streaming flag updates.
 *
 * Default: `true`. When set to `false`, an efficient caching polling
 * mechanism is used. We do not recommend disabling `streaming` unless you
 * have been instructed to do so by LaunchDarkly support. */
LD_EXPORT(void) LDConfigSetStreaming(LDConfig *const config,
    const bool streaming);

/** @brief Only relevant when `streaming` is disabled (set to `false`). Sets
 * the interval between feature flag updates. */
LD_EXPORT(void) LDConfigSetPollingIntervalMillis(LDConfig *const config,
    const int millis);

/** @brief Set the stream uri for connecting to the flag update stream. You
 * probably don't need to set this unless instructed by LaunchDarkly. */
LD_EXPORT(void) LDConfigSetStreamURI(LDConfig *const config,
    const char *const uri);

/** @brief Set the proxy server used for connecting to LaunchDarkly.
 *
 * By default no proxy is used. The URI string should be of the form
 * `socks5://127.0.0.1:9050`. You may read more about how this SDK handles
 * proxy servers by reading the [libcurl](https://curl.haxx.se) documentation
 * on the subject [here](https://ec.haxx.se/libcurl-proxies.html). */
LD_EXPORT(void) LDConfigSetProxyURI(LDConfig *const config,
    const char *const uri);

/** @brief Set whether to verify the authenticity of the peer's certificate on network requests.
 *
 * By default peer verification is enabled. You may read more about what this means
 * by reading the [libcurl](https://curl.haxx.se) documentation
 * on the subject [here](https://curl.haxx.se/libcurl/c/CURLOPT_SSL_VERIFYPEER.html). */
LD_EXPORT(void) LDConfigSetVerifyPeer(LDConfig *const config,
    const bool enabled);

/** @brief Determines whether the `REPORT` or `GET` verb is used for calls to
 * LaunchDarkly. Do not use unless advised by LaunchDarkly. */
LD_EXPORT(void) LDConfigSetUseReport(LDConfig *const config, const bool report);

/** @brief Decide whether the client should fetch feature flag evaluation
 * explanations from LaunchDarkly. */
LD_EXPORT(void) LDConfigSetUseEvaluationReasons(LDConfig *const config,
    const bool reasons);

/** @brief Add a user attribute name to the private list which will not be
 * recorded for all users. */
LD_EXPORT(void) LDConfigAddPrivateAttribute(LDConfig *const config,
    const char *const name);

/** @brief Add another mobile key to the list of secondary environments.
 *
 * Both `name`, and `key` must be unique. You may not add the existing primary
 * environment (the one you used to initialize `LDConfig`). The `name` of the
 * key can later be used in conjunction with `LDClientGetForMobileKey`. This
 * function returns false on failure. */
LD_EXPORT(bool) LDConfigAddSecondaryMobileKey(LDConfig *const config,
    const char *const name, const char *const key);

/** @brief Free an existing `LDConfig` instance.
 *
 * You will likely never use this routine as ownership is transferred to
 * `LDClient` on initialization. */
LD_EXPORT(void) LDConfigFree(LDConfig *const config);

/** @brief Get a reference to the (single, global) client. */
LD_EXPORT(struct LDClient_i *) LDClientGet();

/** @brief Get a reference to a secondary environment established in the
 * configuration.
 *
 * If the environment name does not exist this function
 * returns `NULL`. */
LD_EXPORT(struct LDClient_i *) LDClientGetForMobileKey(
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
LD_EXPORT(struct LDClient_i *) LDClientInit(LDConfig *const config,
    LDUser *const user, const unsigned int maxwaitmilli);

/** @brief Allocate a new user.
 *
 * The user may be modified *until* it is passed
 * to the `LDClientIdentify` or `LDClientInit`. The `key` argument is not
 * required. When `key` is `NULL` then a device specific ID is used. If a
 * device specific ID cannot be obtained then a random fallback is generated. */
LD_EXPORT(LDUser *) LDUserNew(const char *const key);

/** @brief Free a user object */
LD_EXPORT(void) LDUserFree(LDUser *const user);

/** @brief Mark the user as anonymous. */
LD_EXPORT(void) LDUserSetAnonymous(LDUser *const user, const bool anon);

/** @brief Set the user's IP. */
LD_EXPORT(void) LDUserSetIP(LDUser *const user, const char *const str);

/** @brief Set the user's first name. */
LD_EXPORT(void) LDUserSetFirstName(LDUser *const user, const char *const str);

/** @brief Set the user's last name. */
LD_EXPORT(void) LDUserSetLastName(LDUser *const user, const char *const str);

/** @brief Set the user's email. */
LD_EXPORT(void) LDUserSetEmail(LDUser *const user, const char *const str);

/** @brief Set the user's name. */
LD_EXPORT(void) LDUserSetName(LDUser *const user, const char *const str);

/** @brief Set the user's avatar. */
LD_EXPORT(void) LDUserSetAvatar(LDUser *const user, const char *const str);

/** @brief Set the user's secondary key. */
LD_EXPORT(void) LDUserSetSecondary(LDUser *const user, const char *const str);

/** @brief Set custom attributes from JSON object string */
LD_EXPORT(bool) LDUserSetCustomAttributesJSON(LDUser *const user,
    const char *const jstring);

/** @brief Set custom attributes with `LDNode` */
LD_EXPORT(void) LDUserSetCustomAttributes(LDUser *const user,
    LDNode *const custom);

/** @brief Add an attribute name to the private list which will not be
 * recorded. */
LD_EXPORT(void) LDUserAddPrivateAttribute(LDUser *const user,
    const char *const attribute);

/** @brief Get JSON string containing all flags */
LD_EXPORT(char *) LDClientSaveFlags(struct LDClient_i *const client);

/** @brief Set flag store from JSON string */
LD_EXPORT(void) LDClientRestoreFlags(struct LDClient_i *const client,
    const char *const data);

/** @brief Update the client with a new user.
 *
 * The old user is freed. This will re-fetch feature flag settings from
 * LaunchDarkly. For performance reasons, user contexts should not be
 * changed frequently. */
LD_EXPORT(void) LDClientIdentify(struct LDClient_i *const client,
    LDUser *const user);

/** @brief  Send any pending events to the server. They will normally be
 *
 * flushed after a timeout, but may also be flushed manually. This operation
 * does not block. */
LD_EXPORT(void) LDClientFlush(struct LDClient_i *const client);

/** @brief Returns true if the client has been initialized. */
LD_EXPORT(bool) LDClientIsInitialized(struct LDClient_i *const client);

/** @brief Block until initialized up to timeout, returns true if initialized */
LD_EXPORT(bool) LDClientAwaitInitialized(struct LDClient_i *const client,
    const unsigned int timeoutmilli);

/** @brief Returns the offline status of the client. */
LD_EXPORT(bool) LDClientIsOffline(struct LDClient_i *const client);

/** @brief Make the client operate in offline mode. No network traffic. */
LD_EXPORT(void) LDClientSetOffline(struct LDClient_i *const client);

/** @brief Return the client to online mode. */
LD_EXPORT(void) LDClientSetOnline(struct LDClient_i *const client);

/** @brief Enable or disable polling mode */
LD_EXPORT(void) LDClientSetBackground(struct LDClient_i *const client,
    const bool background);

/** @brief Close the client, free resources, and generally shut down.
 *
 * This will additionally close all secondary environments. Do not attempt to
 * manage secondary environments directly. */
LD_EXPORT(void) LDClientClose(struct LDClient_i *const client);

/** @brief Add handler for when client status changes */
LD_EXPORT(void) LDSetClientStatusCallback(void (callback)(int status));

/** @brief Access the flag store must unlock with LDClientUnlockFlags */
LD_EXPORT(LDNode *) LDClientGetLockedFlags(struct LDClient_i *const client);

/** @brief Unlock flag store after direct access */
LD_EXPORT(void) LDClientUnlockFlags(struct LDClient_i *const client);

/** @brief Record a custom event. */
LD_EXPORT(void) LDClientTrack(struct LDClient_i *const client,
    const char *const name);

/** @brief Record a custom event and include custom data. */
LD_EXPORT(void) LDClientTrackData(struct LDClient_i *const client,
    const char *const name, LDNode *const data);

/** @brief  Returns a hash table of all flags. This must be freed with
 * `LDNodeFree`. */
LD_EXPORT(LDNode *) LDAllFlags(struct LDClient_i *const client);

/** @brief Evaluate Bool flag */
LD_EXPORT(bool) LDBoolVariation(struct LDClient_i *const client,
    const char *const featureKey, const bool fallback);

/** @brief Evaluate Int flag */
LD_EXPORT(int) LDIntVariation(struct LDClient_i *const client,
    const char *const featureKey, const int fallback);

/** @brief Evaluate Double flag */
LD_EXPORT(double) LDDoubleVariation(struct LDClient_i *const client,
    const char *const featureKey, const double fallback);

/** @brief Evaluate String flag */
LD_EXPORT(char *) LDStringVariationAlloc(struct LDClient_i *const client,
    const char *const featureKey, const char *const fallback);

/** @brief Evaluate String flag into fixed buffer */
LD_EXPORT(char *) LDStringVariation(struct LDClient_i *const client,
    const char *const featureKey, const char *const fallback,
    char *const resultBuffer, const size_t resultBufferSize);

/** @brief Evaluate JSON flag */
LD_EXPORT(LDNode *) LDJSONVariation(struct LDClient_i *const client,
    const char *const featureKey, const LDNode *const fallback);

/** @brief Evaluate Bool flag with details */
LD_EXPORT(bool) LDBoolVariationDetail(struct LDClient_i *const client,
    const char *const featureKey, const bool fallback,
    LDVariationDetails *const details);

/** @brief Evaluate Int flag with details */
LD_EXPORT(int) LDIntVariationDetail(struct LDClient_i *const client,
    const char *const featureKey, const int fallback,
    LDVariationDetails *const details);

/** @brief Evaluate Double flag with details */
LD_EXPORT(double) LDDoubleVariationDetail(struct LDClient_i *const client,
    const char *const featureKey, const double fallback,
    LDVariationDetails *const details);

/** @brief Evaluate String flag with details */
LD_EXPORT(char *) LDStringVariationAllocDetail(struct LDClient_i *const client,
    const char *const featureKey, const char *const fallback,
    LDVariationDetails *const details);

/** @brief Evaluate String flag into fixed buffer with details */
LD_EXPORT(char *) LDStringVariationDetail(struct LDClient_i *const client,
    const char *const featureKey, const char *const fallback,
    char *const resultBuffer, const size_t resultBufferSize,
    LDVariationDetails *const details);

/** @brief Evaluate JSON flag with details */
LD_EXPORT(LDNode *) LDJSONVariationDetail(struct LDClient_i *const client,
    const char *const key, const LDNode *const fallback,
    LDVariationDetails *const details);

/** @brief Frees memory allocated by the SDK. */
LD_EXPORT(void) LDFree(void *const ptr);

/** @brief Allocate memory for usage by the SDK */
LD_EXPORT(void *) LDAlloc(const size_t bytes);

/** @brief Clear any memory associated with `LDVariationDetails`  */
LD_EXPORT(void) LDFreeDetailContents(LDVariationDetails details);

/** @brief Create a new empty hash.
 *
 * (Implementation note: empty hash is a `NULL` pointer, not indicative of
 * failure). */
LD_EXPORT(LDNode *) LDNodeCreateHash(void);

/** @brief Add Bool value to a hash */
LD_EXPORT(LDNode *) LDNodeAddBool(LDNode **const hash, const char *const key,
    const bool b);

/** @brief Add Number value to a hash */
LD_EXPORT(LDNode *) LDNodeAddNumber(LDNode **const hash, const char *const key,
    const double n);

/** @brief Add String value to a hash */
LD_EXPORT(LDNode *) LDNodeAddString(LDNode **const hash, const char *const key,
    const char *const s);

/** @brief Add Hash value to a hash */
LD_EXPORT(LDNode *) LDNodeAddHash(LDNode **const hash, const char *const key,
    LDNode *const h);

/** @brief Add Array value to a hash */
LD_EXPORT(LDNode *) LDNodeAddArray(LDNode **const hash, const char *const key,
    LDNode *const a);

/** @brief Find a node in a hash. */
LD_EXPORT(LDNode *) LDNodeLookup(const LDNode *const hash,
    const char *const key);

/** @brief Free a hash and all internal memory. */
LD_EXPORT(void) LDNodeFree(LDNode **const hash);

/** @brief Return the number of elements in a hash or array. */
LD_EXPORT(unsigned int) LDNodeCount(const LDNode *const hash);

/** @brief Return a deep copy of a hash */
LD_EXPORT(LDNode *) LDCloneHash(const LDNode *const original);

/** @brief Create a new empty array */
LD_EXPORT(LDNode *) LDNodeCreateArray(void);

/** @brief Append Bool value to an array */
LD_EXPORT(LDNode *) LDNodeAppendBool(LDNode **const array, const bool b);

/** @brief Append Number value to an array */
LD_EXPORT(LDNode *) LDNodeAppendNumber(LDNode **const array, const double n);

/** @brief Append String value to an array */
LD_EXPORT(LDNode *) LDNodeAppendString(LDNode **const array,
    const char *const s);

/** @brief Append existing node to an array */
LD_EXPORT(LDNode *) LDNodeAppendArray(LDNode **const array, LDNode *const a);

/** @brief Add existing node to an array */
LD_EXPORT(LDNode *) LDNodeAppendHash(LDNode **const array, LDNode *const h);

/** @brief Lookup node at a given index in an Array */
LD_EXPORT(LDNode *) LDNodeIndex(const LDNode *const array,
    const unsigned int idx);

/** @brief Deep copy array */
LD_EXPORT(LDNode *) LDCloneArray(const LDNode *const original);

/** @brief Serialize `LDNode` to JSON */
LD_EXPORT(char *) LDNodeToJSON(const LDNode *const node);

/** @brief Deserialize JSON into `LDNode` */
LD_EXPORT(LDNode *) LDNodeFromJSON(const char *const json);

/** @brief Utility to convert a hash to a JSON object. */
LD_EXPORT(char *) LDHashToJSON(const LDNode *const node);

/** @brief Set the log function and log level. Increasing log levels result in
 * increasing output. */
LD_EXPORT(void) LDSetLogFunction(const int userlevel,
    void (userlogfn)(const char *const text));

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
LD_EXPORT(bool) LD_store_filewrite(void *const context, const char *const name,
    const char *const data);

/** @brief Predefined `ld_store_stringreader` for files */
LD_EXPORT(char *) LD_store_fileread(void *const context,
    const char *const name);

/** @brief Feature flag listener callback type.
 *
 * Status 0 for new or updated, 1 for deleted. */
typedef void (*LDlistenerfn)(const char *const flagKey, const int status);

/** @brief Register a callback for when a flag is updated. */
LD_EXPORT(void) LDClientRegisterFeatureFlagListener(
  struct LDClient_i *const client, const char *const flagKey,
  LDlistenerfn listener);

/** @brief Unregister a callback registered with
* `LDClientRegisterFeatureFlagListener` */
LD_EXPORT(bool) LDClientUnregisterFeatureFlagListener(
    struct LDClient_i *const client, const char *const flagKey,
    LDlistenerfn listener);

#if !defined(__cplusplus) && !defined(LD_C_API)
/** @brief Opaque client object **/
typedef struct LDClient_i LDClient;
#endif

#ifdef __cplusplus
}

class LD_EXPORT(LDClient) {
    public:
        static LDClient *Get(void);

        static LDClient *Init(LDConfig *const client, LDUser *const user,
            const unsigned int maxwaitmilli);

        bool isInitialized(void);

        bool awaitInitialized(const unsigned int timeoutmilli);

        bool boolVariation(const std::string &flagKey, const bool fallback);

        int intVariation(const std::string &flagKey, const int fallback);

        double doubleVariation(const std::string &flagKey,
            const double fallback);

        std::string stringVariation(const std::string &flagKey,
            const std::string &fallback);

        char *stringVariation(const std::string &flagKey,
            const std::string &fallback, char *const resultBuffer,
            const size_t resultBufferSize);

        LDNode *JSONVariation(const std::string &flagKey,
            const LDNode *const fallback);

        bool boolVariationDetail(const std::string &flagKey,
            const bool fallback, LDVariationDetails *const details);

        int intVariationDetail(const std::string &flagKey, const int fallback,
            LDVariationDetails *const details);

        double doubleVariationDetail(const std::string &flagKey,
            const double fallback, LDVariationDetails *const details);

        std::string stringVariationDetail(const std::string &flagKey,
            const std::string &fallback, LDVariationDetails *const details);

        char *stringVariationDetail(const std::string &flagKey,
            const std::string &fallback, char *const resultBuffer,
            const size_t resultBufferSize, LDVariationDetails *const details);

        LDNode *JSONVariationDetail(const std::string &flagKey,
            const LDNode *const fallback, LDVariationDetails *const details);

        LDNode *getLockedFlags();

        void unlockFlags();

        LDNode *getAllFlags();

        void setOffline();

        void setOnline();

        bool isOffline();

        void setBackground(const bool background);

        std::string saveFlags();

        void restoreFlags(const std::string &flags);

        void identify(LDUser *);

        void track(const std::string &name);

        void track(const std::string &name, LDNode *const data);

        void flush(void);

        void close(void);

        void registerFeatureFlagListener(const std::string &name,
            LDlistenerfn fn);

        bool unregisterFeatureFlagListener(const std::string &name,
            LDlistenerfn fn);

    private:
        struct LDClient_i *client;
};

#endif

#endif /* C_CLIENT_LDIAPI_H */
