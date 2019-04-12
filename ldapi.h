#ifndef C_CLIENT_LDIAPI_H
#define C_CLIENT_LDIAPI_H

#ifdef _WIN32
    #define LD_EXPORT(x) __declspec(dllexport) x
#else
    #define LD_EXPORT(x) __attribute__((visibility("default"))) x
#endif

#ifdef __cplusplus
#include <string>

extern "C" {
#endif

#include <stdbool.h>

#include "uthash.h"

#define LDPrimaryEnvironmentName "default"

enum ld_log_level {
    LD_LOG_FATAL = 0,
    LD_LOG_CRITICAL,
    LD_LOG_ERROR,
    LD_LOG_WARNING,
    LD_LOG_INFO,
    LD_LOG_DEBUG,
    LD_LOG_TRACE
};

typedef enum {
    LDStatusInitializing = 0,
    LDStatusInitialized,
    LDStatusFailed,
    LDStatusShuttingdown,
    LDStatusShutdown
} LDStatus;

typedef enum {
    LDNodeNone = 0,
    LDNodeString,
    LDNodeNumber,
    LDNodeBool,
    LDNodeHash,
    LDNodeArray,
} LDNodeType;

typedef struct LDNode_i {
    union {
        char *key;
        unsigned int idx;
    };
    LDNodeType type;
    union {
        bool b;
        char *s;
        double n;
        struct LDNode_i *h;
        struct LDNode_i *a;
    };
    UT_hash_handle hh;
    int version;
    int variation;
    int flagversion;
    double track;
    struct LDNode_i* reason;
#ifdef __cplusplus
    struct LDNode_i *lookup(const std::string &key);
    struct LDNode_i *index(unsigned int idx);
#endif
} LDNode;

typedef struct {
    int variationIndex;
    struct LDNode_i* reason;
} LDVariationDetails;

typedef struct LDConfig_i LDConfig;

typedef struct LDUser_i LDUser;

struct LDClient_i;

/* returns true on success */
LD_EXPORT(bool) LDSetString(char **target, const char *value);

LD_EXPORT(LDConfig *) LDConfigNew(const char *mobileKey);
LD_EXPORT(void) LDConfigSetAllAttributesPrivate(LDConfig *config, bool allprivate);
LD_EXPORT(void) LDConfigSetBackgroundPollingIntervalMillis(LDConfig *config, int millis);
LD_EXPORT(void) LDConfigSetAppURI(LDConfig *config, const char *uri);
LD_EXPORT(void) LDConfigSetConnectionTimeoutMillies(LDConfig *config, int millis);
LD_EXPORT(void) LDConfigSetDisableBackgroundUpdating(LDConfig *config, bool disable);
LD_EXPORT(void) LDConfigSetEventsCapacity(LDConfig *config, int capacity);
LD_EXPORT(void) LDConfigSetEventsFlushIntervalMillis(LDConfig *config, int millis);
LD_EXPORT(void) LDConfigSetEventsURI(LDConfig *config, const char *uri);
LD_EXPORT(void) LDConfigSetMobileKey(LDConfig *config, const char *key);
LD_EXPORT(void) LDConfigSetOffline(LDConfig *config, bool offline);
LD_EXPORT(void) LDConfigSetStreaming(LDConfig *config, bool streaming);
LD_EXPORT(void) LDConfigSetPollingIntervalMillis(LDConfig *config, int millis);
LD_EXPORT(void) LDConfigSetStreamURI(LDConfig *config, const char *uri);
LD_EXPORT(void) LDConfigSetProxyURI(LDConfig *config, const char *uri);
LD_EXPORT(void) LDConfigSetUseReport(LDConfig *config, bool report);
LD_EXPORT(void) LDConfigSetUseEvaluationReasons(LDConfig *config, bool reasons);
LD_EXPORT(void) LDConfigAddPrivateAttribute(LDConfig *config, const char *name);
LD_EXPORT(bool) LDConfigAddSecondaryMobileKey(LDConfig *config, const char *name, const char *key);
LD_EXPORT(void) LDConfigFree(LDConfig *config);

/* must have already called LDClientInit to receive valid client */
LD_EXPORT(struct LDClient_i *) LDClientGet();
LD_EXPORT(struct LDClient_i *) LDClientGetForMobileKey(const char *keyName);
/* create the global client singleton */
LD_EXPORT(struct LDClient_i *) LDClientInit(LDConfig *, LDUser *, unsigned int maxwaitmilli);

LD_EXPORT(LDUser *) LDUserNew(const char *);

LD_EXPORT(void) LDUserSetAnonymous(LDUser *user, bool anon);
LD_EXPORT(void) LDUserSetIP(LDUser *user, const char *str);
LD_EXPORT(void) LDUserSetFirstName(LDUser *user, const char *str);
LD_EXPORT(void) LDUserSetLastName(LDUser *user, const char *str);
LD_EXPORT(void) LDUserSetEmail(LDUser *user, const char *str);
LD_EXPORT(void) LDUserSetName(LDUser *user, const char *str);
LD_EXPORT(void) LDUserSetAvatar(LDUser *user, const char *str);
LD_EXPORT(void) LDUserSetSecondary(LDUser *user, const char *str);

LD_EXPORT(bool) LDUserSetCustomAttributesJSON(LDUser *user, const char *jstring);
LD_EXPORT(void) LDUserSetCustomAttributes(LDUser *user, LDNode *custom);
LD_EXPORT(void) LDUserAddPrivateAttribute(LDUser *user, const char *attribute);

LD_EXPORT(char *) LDClientSaveFlags(struct LDClient_i *client);
LD_EXPORT(void) LDClientRestoreFlags(struct LDClient_i *client, const char *data);

LD_EXPORT(void) LDClientIdentify(struct LDClient_i *, LDUser *);

LD_EXPORT(void) LDClientFlush(struct LDClient_i *client);
LD_EXPORT(bool) LDClientIsInitialized(struct LDClient_i *);
/* block until initialized up to timeout, returns true if initialized */
LD_EXPORT(bool) LDClientAwaitInitialized(struct LDClient_i *client, unsigned int timeoutmilli);
LD_EXPORT(bool) LDClientIsOffline(struct LDClient_i *);
LD_EXPORT(void) LDClientSetOffline(struct LDClient_i *);
LD_EXPORT(void) LDClientSetOnline(struct LDClient_i *);
LD_EXPORT(void) LDClientSetBackground(struct LDClient_i *client, bool background);
LD_EXPORT(void) LDClientClose(struct LDClient_i *);

LD_EXPORT(void) LDSetClientStatusCallback(void (callback)(int));

/* Access the flag store must unlock with LDClientUnlockFlags */
LD_EXPORT(LDNode *) LDClientGetLockedFlags(struct LDClient_i *client);
LD_EXPORT(void) LDClientUnlockFlags(struct LDClient_i *client);

LD_EXPORT(void) LDClientTrack(struct LDClient_i *client, const char *name);
LD_EXPORT(void) LDClientTrackData(struct LDClient_i *client, const char *name, LDNode *data);

/* returns a hash table of existing flags, must be freed with LDNodeFree */
LD_EXPORT(LDNode *) LDAllFlags(struct LDClient_i *const client);

LD_EXPORT(bool) LDBoolVariation(struct LDClient_i *, const char *, bool);
LD_EXPORT(int) LDIntVariation(struct LDClient_i *, const char *, int);
LD_EXPORT(double) LDDoubleVariation(struct LDClient_i *, const char *, double);
LD_EXPORT(char *) LDStringVariationAlloc(struct LDClient_i *, const char *, const char *);
LD_EXPORT(char *) LDStringVariation(struct LDClient_i *, const char *, const char *, char *, size_t);
LD_EXPORT(LDNode *) LDJSONVariation(struct LDClient_i *client, const char *key, const LDNode *);

LD_EXPORT(bool) LDBoolVariationDetail(struct LDClient_i *, const char *, bool, LDVariationDetails *);
LD_EXPORT(int) LDIntVariationDetail(struct LDClient_i *, const char *, int, LDVariationDetails *);
LD_EXPORT(double) LDDoubleVariationDetail(struct LDClient_i *, const char *, double, LDVariationDetails *);
LD_EXPORT(char *) LDStringVariationAllocDetail(struct LDClient_i *, const char *, const char *, LDVariationDetails *);
LD_EXPORT(char *) LDStringVariationDetail(struct LDClient_i *, const char *, const char *, char *, size_t, LDVariationDetails *);
LD_EXPORT(LDNode *) LDJSONVariationDetail(struct LDClient_i *client, const char *key, const LDNode *, LDVariationDetails *);

LD_EXPORT(void) LDFree(void *);
LD_EXPORT(void *) LDAlloc(size_t amt);
LD_EXPORT(void) LDFreeDetailContents(LDVariationDetails details);

/* functions for working with (JSON) nodes (aka hash tables) */
LD_EXPORT(LDNode *) LDNodeCreateHash(void);
LD_EXPORT(LDNode *) LDNodeAddBool(LDNode **hash, const char *key, bool b);
LD_EXPORT(LDNode *) LDNodeAddNumber(LDNode **hash, const char *key, double n);
LD_EXPORT(LDNode *) LDNodeAddString(LDNode **hash, const char *key, const char *s);
LD_EXPORT(LDNode *) LDNodeAddHash(LDNode **hash, const char *key, LDNode *h);
LD_EXPORT(LDNode *) LDNodeAddArray(LDNode **hash, const char *key, LDNode *a);
LD_EXPORT(LDNode *) LDNodeLookup(const LDNode *hash, const char *key);
LD_EXPORT(void) LDNodeFree(LDNode **hash);
LD_EXPORT(unsigned int) LDNodeCount(const LDNode *hash);
LD_EXPORT(LDNode *) LDCloneHash(const LDNode *original);
/* functions for treating nodes as arrays */
LD_EXPORT(LDNode *) LDNodeCreateArray(void);
LD_EXPORT(LDNode *) LDNodeAppendBool(LDNode **array, bool b);
LD_EXPORT(LDNode *) LDNodeAppendNumber(LDNode **array, double n);
LD_EXPORT(LDNode *) LDNodeAppendString(LDNode **array, const char *s);
LD_EXPORT(LDNode *) LDNodeAppendArray(LDNode **array, LDNode *a);
LD_EXPORT(LDNode *) LDNodeAppendHash(LDNode **array, LDNode *h);
LD_EXPORT(LDNode *) LDNodeIndex(const LDNode *array, unsigned int idx);
LD_EXPORT(LDNode *) LDCloneArray(const LDNode *original);
/* functions for converting nodes to / from JSON */
LD_EXPORT(char *) LDNodeToJSON(const LDNode *node);
LD_EXPORT(LDNode *) LDNodeFromJSON(const char *json);
LD_EXPORT(char *) LDHashToJSON(const LDNode *node);

LD_EXPORT(void) LDSetLogFunction(int userlevel, void (userlogfn)(const char *));

/*
 * store interface. open files, read/write strings, ...
 */
typedef bool (*LD_store_stringwriter)(void *context, const char *name, const char *data);
typedef char *(*LD_store_stringreader)(void *context, const char *name);

LD_EXPORT(void) LD_store_setfns(void *context, LD_store_stringwriter, LD_store_stringreader);

LD_EXPORT(bool) LD_store_filewrite(void *context, const char *name, const char *data);
LD_EXPORT(char *) LD_store_fileread(void *context, const char *name);

/*
 * listener function for flag changes.
 * arguments:
 * flag name
 * change type - 0 for new or updated, 1 for deleted
 */
typedef void (*LDlistenerfn)(const char *, int);
/*
 * register a new listener.
 */
LD_EXPORT(void) LDClientRegisterFeatureFlagListener(struct LDClient_i *, const char *, LDlistenerfn);
LD_EXPORT(bool) LDClientUnregisterFeatureFlagListener(struct LDClient_i *, const char *, LDlistenerfn);

#if !defined(__cplusplus) && !defined(LD_C_API)
typedef struct LDClient_i LDClient;
#endif

#ifdef __cplusplus
}

class LD_EXPORT(LDClient) {
    public:
        static LDClient *Get(void);
        static LDClient *Init(LDConfig *, LDUser *, unsigned int maxwaitmilli);

        bool isInitialized(void);
        bool awaitInitialized(unsigned int timeoutmilli);

        bool boolVariation(const std::string &, bool);
        int intVariation(const std::string &, int);
        double doubleVariation(const std::string &, double);
        std::string stringVariation(const std::string &, const std::string &);
        char *stringVariation(const std::string &, const std::string &, char *, size_t);
        LDNode *JSONVariation(const std::string &, const LDNode *);

        bool boolVariationDetail(const std::string &, bool, LDVariationDetails *);
        int intVariationDetail(const std::string &, int, LDVariationDetails *);
        double doubleVariationDetail(const std::string &, double, LDVariationDetails *);
        std::string stringVariationDetail(const std::string &, const std::string &, LDVariationDetails *);
        char *stringVariationDetail(const std::string &, const std::string &, char *, size_t, LDVariationDetails *);
        LDNode *JSONVariationDetail(const std::string &, const LDNode *, LDVariationDetails *);

        LDNode *getLockedFlags();
        void unlockFlags();

        LDNode *getAllFlags();

        void setOffline();
        void setOnline();
        bool isOffline();
        void setBackground(bool background);

        std::string saveFlags();
        void restoreFlags(const std::string &);

        void identify(LDUser *);

        void track(const std::string &name);
        void track(const std::string &name, LDNode *data);

        void flush(void);
        void close(void);

        void registerFeatureFlagListener(const std::string &name, LDlistenerfn fn);
        bool unregisterFeatureFlagListener(const std::string &name, LDlistenerfn fn);

    private:
        struct LDClient_i *client;
};

#endif

#endif /* C_CLIENT_LDIAPI_H */
