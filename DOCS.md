
# Introduction

This document serves as a reference guide for the LaunchDarkly C SDK API. It documents the functions and structs declared in `ldapi.h`. For complete installation and setup instructions, see the [C SDK Reference](https://docs.launchdarkly.com/v2.0/docs/c-sdk-reference).

## Configuration

```C
LDConfig *LDConfigNew(const char *authkey)
```

Creates a new default configuration. `authkey` is required.

The configuration object is intended to be modified until it is passed to `LDClientInit`, at which point it should no longer be modified. Here are the modifiable attributes defined in `LDConfig`:

```C
void LDConfigSetAllAttributesPrivate(LDConfig *config, bool allprivate);
```

Marks all user attributes private.

```C
void LDConfigSetBackgroundPollingIntervalMillis(LDConfig *config, int millis);
```

Sets the interval in milliseconds between polls for flag updates when your app is in the background.

```C
void LDConfigSetAppURI(LDConfig *config, const char *uri);
```

Set the URI for connecting to LaunchDarkly. You probably don't need to set this unless instructed by LaunchDarkly.

```C
void LDConfigSetConnectionTimeoutMillies(LDConfig *config, int millis);
```

Sets the timeout in milliseconds when connecting to LaunchDarkly.

```C
void LDConfigSetDisableBackgroundUpdating(LDConfig *config, bool disable);
```

Disables feature flag updates when your app is in the background.

```C
void LDConfigSetEventsCapacity(LDConfig *config, int capacity);
```

Sets the max number of events to queue before sending them to LaunchDarkly.

```C
void LDConfigSetEventsFlushIntervalMillis(LDConfig *config, int millis);
```

 Sets the maximum amount of time in milliseconds to wait in between sending analytics events to LaunchDarkly.

```C
void LDConfigSetEventsURI(LDConfig *config, const char *uri);
```

Set the events uri for sending analytics to LaunchDarkly. You probably don't need to set this unless instructed by LaunchDarkly.

```C
void LDConfigSetMobileKey(LDConfig *config, const char *key);
```

Sets the key for authenticating with LaunchDarkly. This is required unless you're using the client in offline mode.

```C
void LDConfigSetOffline(LDConfig *config, bool offline);
```

Configures the client for offline mode. In offline mode, no external network connections are made.

```C
void LDConfigSetStreaming(LDConfig *config, bool streaming);
```

Enables or disables real-time streaming flag updates. Default: `true`. When set to `false`, an efficient caching polling mechanism is used. We do not recommend disabling `streaming` unless you have been instructed to do so by LaunchDarkly support.

```C
void LDConfigSetPollingIntervalMillis(LDConfig *config, int millis);
```

Only relevant when `streaming` is disabled (set to `false`). Sets the interval between feature flag updates.

```C
void LDConfigSetStreamURI(LDConfig *config, const char *uri);
```

Set the stream uri for connecting to the flag update stream. You probably don't need to set this unless instructed by LaunchDarkly.

```C
void LDConfigSetUseReport(LDConfig *config, bool report);
```

Determines whether the `REPORT` or `GET` verb is used for calls to LaunchDarkly. Do not use unless advised by LaunchDarkly.

```C
void LDConfigSetProxyURI(LDConfig *config, const char *uri);
```

Set the proxy server used for connecting to LaunchDarkly. By default no proxy is used. The URI string should be of the form `socks5://127.0.0.1:9050`. You may read more about how this SDK handles proxy servers by reading the [libcurl](https://curl.haxx.se) documentation on the subject [here](https://ec.haxx.se/libcurl-proxies.html).

## Users

```C
LDUser *LDUserNew(const char *key);
```

Allocate a new user. The user may be modified *until* it is passed to the `LDClientIdentify` or `LDClientInit`. The `key` argument is not required. When `key` is `NULL` then a device specific ID is used. If a device specific ID cannot be obtained then a random fallback is generated.

```C
void LDUserSetAnonymous(LDUser *user, bool anon);
```

Mark the user as anonymous.

```C
void LDUserSetIP(LDUser *user, const char *str);
```

Set the user's IP.

```C
void LDUserSetFirstName(LDUser *user, const char *str);
```

Set the user's first name.

```C
void LDUserSetLastName(LDUser *user, const char *str);
```

Set the user's last name.

```C
void LDUserSetEmail(LDUser *user, const char *str);
```

Set the user's email.

```C
void LDUserSetName(LDUser *user, const char *str);
```

Set the user's name.

```C
void LDUserSetAvatar(LDUser *user, const char *str);
```

Set the user's avatar.

```C
bool LDUserSetCustomAttributesJSON(LDUser *user, const char *jstring);
void LDUSerSetCustomAttributes(LDUser *user, LDNode *custom);
```

Helper function to set custom attributes for the user. The custom field
may be built programmatically using the LDNode functions (see below), or
may be set all at once with a json string.

```C
void LDUserAddPrivateAttribute(LDUser *user, const char *name);
```

Add an attribute name to the private list which will not be recorded.

```C
void LDClientIdentify(LDClient *client, LDUser *user);
```

Update the client with a new user. The old user is freed. This will re-fetch feature flag settings from LaunchDarkly-- for performance reasons, user contexts should not be changed frequently.

## Client lifecycle management

```C
LDClient *LDClientInit(LDConfig *config, LDUser *user, unsigned int maxwaitmilli);
```

Initialize the client with the config and user. After this call, the `config` and `user` must not be modified. There is only ever one `LDClient`. The parameter `maxwaitmilli` indicates the maximum amount of time the client will wait to be fully initialized. If the timeout is hit the client will be available for feature flag evaluation but the results will be fallbacks. The client will continue attempting to connect to LaunchDarkly in the background. If `maxwaitmilli` is set to `0` then `LDClientInit` will wait indefinitely.

Only a single initialized client from `LDClientInit` may exist at one time. To initialize another instance you must first cleanup the previous client with `LDClientClose`. Should you initialize with `LDClientInit` while another client exists `abort` will be called.

```C
LDClient *LDClientGet();
```

Get a reference to the (single, global) client.

```C
void LDClientClose(LDClient *client);
```

Close the client, free resources, and generally shut down.

```C
bool LDClientIsInitialized(LDClient *client);
```

Returns true if the client has been initialized.

```C
bool LDClientAwaitInitialized(LDClient *client, unsigned int timeoutmilli);
```

Block until initialized up to timeout, returns true if initialized.

```C
void LDClientFlush(LDClient *client);
```

Send any pending events to the server. They will normally be flushed after a timeout, but may also be flushed manually. This operation does not block.

```C
void LDClientSetOffline(LDClient *client);
```

Make the client operate in offline mode. No network traffic.

```C
void LDClientSetOnline(LDClient *client);
```

Return the client to online mode.

```C
bool LDClientIsOffline();
```

Returns the offline status of the client.

```C
void LDSetClientStatusCallback(void (callback)(LDStatus status));
```

Set a callback function for client status changes. These are major status changes only, not updates to the feature Node. Current status codes:

```c
typedef enum {
    LDStatusInitializing, //Initializing. Flags may be evaluated at this time
    LDStatusInitialized, //Ready. The client has received an initial feature Node from the server and is ready to proceed
    LDStatusFailed, //Offline. The client has been shut down, likely due to a permission failure
    LDStatusShuttingdown, //In the process of shutting down. Flags should not be evaluated at this time
    LDStatusShutdown //The client has fully shutdown. Interacting with the client object is not safe
} LDStatus;
```

## Feature flags

```C
bool LDBoolVariation(LDClient *client, const char *featurekey , bool defaultvalue);
int LDIntVariation(LDClient *client, const char *featurekey, int defaultvalue);
double LDDoubleVariation(LDClient *client, const char *featurekey, double defaultvalue);
```

Ask for a bool, int, or double flag, respectively. Return the default if not
found.

```C
char *LDStringVariationAlloc(LDClient *client, const char *featurekey, const char *defaultvalue);
char *LDStringVariation(LDClient *client, const char *featurekey, const char *defaultvalue,
    char *buffer, size_t size);
```

Ask for a string flag. The first version allocates memory on every call. This
must then be freed with LDFree.
To avoid allocations, the second function fills a caller provided buffer. Up to
size bytes will be copied into buffer, truncating if necessary.
Both functions return a pointer.

```C
LDNode *LDJSONVariation(LDClient *client, const char *featurekey, const LDNode *defaultvalue);
```

Ask for a JSON variation, returned as a parsed tree of LDNodes. You must free the result with `LDNodeFree`. See also `LDNodeLookup`.

```C
typedef void (*LDlistenerfn)(const char *featurekey, int update);
bool LDClientRegisterFeatureFlagListener(LDClient *client, const char *featurekey, LDlistenerfn);
bool LDClientUnregisterFeatureFlagListener(LDClient *client, const char *featurekey, LDlistenerfn);
```

Register and unregister callbacks when features change. The name argument indicates the changed value. The update argument is 0 for new or updated and 1 for deleted.

```C
LDNode *LDClientGetLockedFlags(LDClient *client);
void LDClientUnlockFlags(LDClient *client);
```

Directly access all flags. This locks the client until the flags are unlocked.

```C
LDNode *LDAllFlags(LDClient *client);
```

Returns a hash table of all flags. This must be freed with `LDNodeFree`.

```C
void LDClientTrack(LDClient *client, const char *name);
void LDClientTrackData(LDClient *client, const char *name, LDNode *data);
```

Record a custom event.

## LDNode JSON interface

The LD client uses JSON to communicate, which is represented as LDNode
structures. Both arrays and hashes (objects) are supported.

```C
LDNode *LDNodeCreateHash();
```

Create a new empty hash.
(Implementation note: empty hash is a NULL pointer, not indicative of failure).

```C
void LDNodeAddBool(LDNode **hash, const char *key, bool b);
void LDNodeAddNumber(LDNode **hash, const char *key, double n);
void LDNodeAddString(LDNode **hash, const char *key, const char *s);
void LDNodeAddNode(LDNode **hash, const char *key, LDNode *m);
```

Add a new node to the hash table.
Memory ownership: The string `s` will be duplicated internally. The Node m
is _not_ duplicated. It will be owned by the containing hash.

```C
LDNode *LDNodeLookup(const LDNode *hash, const char *key);
```

Find a node in a hash. See below for structure.

```C
void LDNodeFree(LDNode **hash);
```

Free a hash and all internal memory.

In addition to hash tables, arrays are also supported.

```C
LDNode *LDNodeArray();
```

Create an empty array.

```C
void LDNodeAppendBool(LDNode **array, bool b);
void LDNodeAppendNumber(LDNode **array, double n);
void LDNodeAppendString(LDNode **array, const char *s);
```

Add a bool, number, or string to an array.

```C
LDNode *LDCloneHash(const LDNode *hash);
LDNode *LDCloneArray(const LDNode *array);
```
Return a deep copy of the originals.

```C
LDNode *LDNodeIndex(const LDNode *array, unsigned int idx);
```

Retrieve the element at index idx.

```C
unsigned int LDNodeCount(const LDNode *hash);
```

Return the number of elements in a hash or array.

A Node node will have a type, one of string, number, bool, hash, or array.
The corresponding union field, s, n, b, h, or a will be set.

```C
typedef enum {
    LDNodeNone = 0,
    LDNodeString,
    LDNodeNumber,
    LDNodeBool,
    LDNodeHash,
    LDNodeArray,
} LDNodeType;

typedef struct {
    char *key;
    LDNodeType type;
    union {
        bool b;
        char *s;
        double n;
        LDNode *h;
        LDNode *a;
    };
} LDNode;
```

## Store interface

The client library supports persisting data between sessions or when internet
connectivity is intermittent. This is exposed to the application via the
LD_store interface which consists of two main functions.

```C
void LD_store_setfns(void *context, LD_store_stringwriter, LD_store_stringreader);
```

`LD_store_setfns` sets the functions to be used. Reader and writer may optionally be `NULL`. The provided opaque context is passed to each open call.

```C
typedef bool (*LD_store_stringwriter)(void *context, const char *name, const char *data);
```

Should write the `data` using the associated `context` to `name`. Returns `true` for success.

```C
typedef char *(*LD_store_stringreader)(void *context, const char *name);
```

Read a string from the `name` associated with the `context`. Returns `NULL` on failure. Memory returned from the reader must come from `LDAlloc`.

A simple set of functions that operate on POSIX files is provided:

```C
bool LD_store_filewrite(void *context, const char *name, const char *data);
char *LD_store_fileread(void *context, const char *name);
```

## Logging

```C
void LDSetLogFunction(int userlevel, void (userlogfn)(const char *))
```

Set the log function and log level. Increasing log levels result in increasing output. The current log levels are:

```C
enum ld_log_level {
    LD_LOG_FATAL = 0,
    LD_LOG_CRITICAL,
    LD_LOG_ERROR,
    LD_LOG_WARNING,
    LD_LOG_INFO,
    LD_LOG_DEBUG,
    LD_LOG_TRACE
};
```

## Other utilities

```C
void LDFree(void *)
```

Frees memory allocated by the SDK.

## C++ interface

A C++ interface to LDClient is also available. Other C functions remain available.

```C++
class LDClient {
```

```C++
        static LDClient *Get();
```

Return the LDClient singleton.

```C++
        static LDClient *Init(LDConfig *config, LDUser *user);
        bool isInitialized();
        bool awaitInitialized(unsigned int timeoutmilli);
```

Initialize the client and functions to check the initialization status.

```C++
        bool boolVariation(const std::string &featurekey, bool defaultvalue);
        int intVariation(const std::string &featurekey, int defaultvalue);
        std::string stringVariation(const std::string &featurekey, const std::string &defaultvalue);
        char *stringVariation(const std::string &featurekey, const std::string &defaultvalue,
            char *buffer, size_t size);
```

Functions to ask for variations.

```C++
        LDNode *JSONVariation(const std::string &featurekey, const LDNode *defaultvalue);
```

Request a JSON variation. It must be freed.

```C++
        LDNode *getLockedFlags();
        void unlockFlags();
        LDNode *getAllFlags();
```

Functions to access or copy the flag store.

```C++
        void identify(LDUser *user);
```

Switch user and generate identify event, ownership of the `user` object is transferred to the client. This causes a reconnection to the LaunchDarkly servers and refreshes the flag store in the background. This should not be called frequently because of the performance implications. You may want to call `awaitInitialized` to wait until the updated flag store is ready.

```C++
        void setOffline();
        void setOnline();
        bool isOffline();
```

```C++
        std::string saveFlags();
        void restoreFlags(const std::string &flagsjson);
```

```C++
        void flush();
        void close();
```

```C++
}
```

In C++, LDNode has the following member functions.

```C++
class LDNode {
```

```C++
    LDNode *lookup(const std::string &key);
```

Find a subnode.

```C++
    LDNode *index(unsigned int idx);
```

Retrieve the element at index idx.

```C++
}
```
