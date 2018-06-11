
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


## Users

```C
LDUser *LDUserNew(const char *key);
```
Allocate a new user. The user may be modified *until* it is passed to the `LdClientIdentify` or `LDClientInit`. The `key` argument is required.

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
void LDUserAddPrivateAttribute(LDUser *, const char *name);
```
Add an attribute name to the private list which will not be recorded.


```C
void LDClientIdentify(LDClient *, LDUser *);
```
Update the client with a new user. The old user is freed. This will re-fetch feature flag settings from LaunchDarkly-- for performance reasons, user contexts should not be changed frequently.

## Client lifecycle management

```C
LDClient *LDClientInit(LDConfig *config, LDUser *user);
```
Initialize the client with the config and user. After this call, the `config`
and `user` must not be modified. May be called more than once to change
`config`, in which case the previous `config` is freed. There is only ever one `LDClient`.

```C
LDClient *LDClientGet(void);
```
Get a reference to the (single, global) client.

```C
void LDClientClose(LDClient *);
```
Close the client, free resources, and generally shut down.

```C
bool LDClientIsInitialized(LDClient *);
```
Returns true if the client has been initialized.

```C
void LDClientFlush(LDClient *client);
```
Send any pending events to the server. They will normally be flushed after a
timeout, but may also be flushed manually.

```C
void LDClientSetOffline(LDClient *);
```
Make the client operate in offline mode. No network traffic.

```C
void LDClientSetOnline(LDClient *);
```
Return the client to online mode.

```C
bool LDClientIsOffline(void);
```
Returns the offline status of the client.

```C
void LDSetClientStatusCallback(void (callback)(int status));
```
Set a callback function for client status changes. These are major
status changes only, not updates to the feature Node.
Current status code:
0 - Offline. The client has been shut down, likely due to a permission failure.
1 - Ready. The client has received an initial feature Node from the server
    and is ready to proceed.


## Feature flags


```C
bool LDBoolVariation(LDClient *, const char *name , bool default);
int LDIntVariation(LDClient *, const char *name, int default);
double LDDoubleVariation(LDClient *, const char *name, double default);
```
Ask for a bool, int, or double flag, respectively. Return the default if not
found.

```C
char *LDStringVariationAlloc(LDClient *, const char *name, const char *def);
char *LDStringVariation(LDClient *, const char *name, const char *default,
    char *buffer, size_t size);
```
Ask for a string flag. The first version allocates memory on every call. This
must then be freed with LDFree.
To avoid allocations, the second function fills a caller provided buffer. Up to
size bytes will be copied into buffer, truncating if necessary.
Both functions return a pointer.

```C
LDNode *LDJSONVariation(LDClient *client, const char *name, LDNode *default);
```
Ask for a JSON variation, returned as a parsed tree of LDNodes.
The node returned is an internal data structure of the client. After examination,
it must be released by calling `LDJSONRelease()` which will unlock the client.
See also `LDNodeLookup`.


```C
typedef void (*LDlistenerfn)(const char *name, int update);
bool LDClientRegisterFeatureFlagListener(LDClient *client, const char *name, LDlistenerfn);
bool LDClientUnregisterFeatureFlagListener(LDClient *client, const char *name, LDlistenerfn);
```
Register and unregister callbacks when features change.
The name argument indicates the changed value.
The update argument is 0 for new or updated and 1 for deleted.

## Node interface


The LD client uses JSON to communicate, which is represented as LDNode
structures. Both arrays and hashes (objects) are supported.

```C
LDNode *LDNodeCreateHash(void);
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
LDNode *LDNodeLookup(LDNode *hash, const char *key);
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
LDNode *LDNodeIndex(LDNode *array, unsigned int idx);
```
Retrieve the element at index idx.

```C
unsigned int LDNodeCount(LDNode *hash);
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
LD_store interface which consists of four functions.

```C
void
LD_store_setfns(void *context, LD_store_opener, LD_store_stringwriter,
    LD_store_stringreader, LD_store_closer)
```
`LD_store_setfns` sets the functions to be used. The opener and closer
are required, but reader and writer may optionally be `NULL`. The
provided opaque context is passed to each open call.

```C
typedef void *(*LD_store_opener)(void *context, const char *name,
    const char *mode, size_t predictedlen);
```
The opener function should return a handle to an open file.
context is as set above. name identifies the data requested. mode
is an fopen style string, "r" for reading and "w" for writing.
predictedlen is the expected length of data to be read or written.
It returns a handle for further operations, or NULL for failure.

```C
typedef bool (*LD_store_stringwriter)(void *handle, const char *data);
```
Should write the data to the handle. Returns true for success.

```C
typedef const char *(*LD_store_stringreader)(void *handle);
```
Read a string from the handle. Returns NULL on failure.
Memory is to be managed by the handle.

```C
typedef void (*LD_store_closer)(void *handle);
```
Close a handle and free resources (e.g. the string returned by stringreader).

A simple set of functions that operate on POSIX files is provided:

```C
void *LD_store_fileopen(void *, const char *name, const char *mode, size_t len);
bool LD_store_filewrite(void *h, const char *data);
const char *LD_store_fileread(void *h);
void LD_store_fileclose(void *h);
```

## Logging

```C
void LDSetLogFunction(int userlevel, void (userlogfn)(const char *))
```
Set the log function and log level. Increasing log levels result in increasing output.


## Other utilities

```C
void LDFree(void *)
```
Frees a string.

## C++ interface


A C++ interface to LDClient is also available. Other C functions remain available.

```C++
class LDClient {
```

```C++
        static LDClient *Get(void);
```
        Return the LDClient singleton.

```C++
        static LDClient *Init(LDConfig *, LDUser *);
```
        Initialize the client.

```C++
        bool boolVariation(const std::string &, bool);
        int intVariation(const std::string &, int);
        std::string stringVariation(const std::string &, const std::string &);
        char *stringVariation(const std::string &, const std::string &, char *, size_t);
```
        Functions to ask for variations.

```C++
        LDNode *JSONVariation(const std::string &, LDNode *);
```
        Request a JSON variation. It must be released.

```C++
        void setOffline();
        void setOnline();
        bool isOffline();
```

```C++      
        std::string saveFlags();
        void restoreFlags(const std::string &);
```

```C++
        void flush(void);
        void close(void);
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
    void release(void);
```
    Release a node, as returned from LDClient::JSONVariation.

```C++
}
```