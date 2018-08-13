
#include <stdbool.h>

#include "uthash.h"

#ifdef __cplusplus
#include <string>

extern "C" {
#endif

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
    double track;
#ifdef __cplusplus
    struct LDNode_i *lookup(const std::string &key);
    struct LDNode_i *index(unsigned int idx);
    void release(void);
#endif
} LDNode;

typedef struct LDConfig_i LDConfig;

typedef struct LDUser_i LDUser;

struct LDClient_i;

#if !defined(__cplusplus) && !defined(LD_C_API)
typedef struct LDClient_i LDClient;
#endif


void LDSetString(char **, const char *);

LDConfig *LDConfigNew(const char *);
void LDConfigSetAllAttributesPrivate(LDConfig *config, bool allprivate);
void LDConfigSetBackgroundPollingIntervalMillis(LDConfig *config, int millis);
void LDConfigSetAppURI(LDConfig *config, const char *uri);
void LDConfigSetConnectionTimeoutMillies(LDConfig *config, int millis);
void LDConfigSetDisableBackgroundUpdating(LDConfig *config, bool disable);
void LDConfigSetEventsCapacity(LDConfig *config, int capacity);
void LDConfigSetEventsFlushIntervalMillis(LDConfig *config, int millis);
void LDConfigSetEventsURI(LDConfig *config, const char *uri);
void LDConfigSetMobileKey(LDConfig *config, const char *key);
void LDConfigSetOffline(LDConfig *config, bool offline);
void LDConfigSetStreaming(LDConfig *config, bool streaming);
void LDConfigSetPollingIntervalMillis(LDConfig *config, int millis);
void LDConfigSetStreamURI(LDConfig *config, const char *uri);
void LDConfigSetUseReport(LDConfig *config, bool report);
void LDConfigAddPrivateAttribute(LDConfig *, const char *name);


struct LDClient_i *LDClientInit(LDConfig *, LDUser *);
struct LDClient_i *LDClientGet(void);


LDUser *LDUserNew(const char *);

void LDUserSetAnonymous(LDUser *user, bool anon);
void LDUserSetIP(LDUser *user, const char *str);
void LDUserSetFirstName(LDUser *user, const char *str);
void LDUserSetLastName(LDUser *user, const char *str);
void LDUserSetEmail(LDUser *user, const char *str);
void LDUserSetName(LDUser *user, const char *str);
void LDUserSetAvatar(LDUser *user, const char *str);

bool LDUserSetCustomAttributesJSON(LDUser *user, const char *jstring);
void LDUSerSetCustomAttributes(LDUser *user, LDNode *custom);
void LDUserAddPrivateAttribute(LDUser *, const char *name);

char *LDClientSaveFlags(struct LDClient_i *client);
void LDClientRestoreFlags(struct LDClient_i *client, const char *data);

void LDClientIdentify(struct LDClient_i *, LDUser *);

void LDClientFlush(struct LDClient_i *client);
bool LDClientIsInitialized(struct LDClient_i *);
bool LDClientIsOffline(struct LDClient_i *);
void LDClientSetOffline(struct LDClient_i *);
void LDClientSetOnline(struct LDClient_i *);
void LDClientSetBackground(LDClient *client, bool background);
void LDClientClose(struct LDClient_i *);

void LDSetClientStatusCallback(void (callback)(int));

LDNode *LDClientGetLockedFlags(LDClient *client);
void LDClientPutLockedFlags(LDClient *client, LDNode *flags);

void LDClientTrack(LDClient *client, const char *name);
void LDClientTrackData(LDClient *client, const char *name, LDNode *data);

bool LDBoolVariation(struct LDClient_i *, const char *, bool);
int LDIntVariation(struct LDClient_i *, const char *, int);
double LDDoubleVariation(struct LDClient_i *, const char *, double);
char *LDStringVariationAlloc(struct LDClient_i *, const char *, const char *);
char *LDStringVariation(struct LDClient_i *, const char *, const char *, char *, size_t);
LDNode *LDJSONVariation(struct LDClient_i *client, const char *key, LDNode *);
void LDJSONRelease(LDNode *m);

void LDFree(void *);
void *LDAlloc(size_t amt);

/* functions for working with (JSON) nodes (aka hash tables) */
LDNode *LDNodeCreateHash(void);
LDNode * LDNodeAddBool(LDNode **hash, const char *key, bool b);
LDNode * LDNodeAddNumber(LDNode **hash, const char *key, double n);
LDNode * LDNodeAddString(LDNode **hash, const char *key, const char *s);
LDNode * LDNodeAddHash(LDNode **hash, const char *key, LDNode *h);
LDNode * LDNodeAddArray(LDNode **hash, const char *key, LDNode *a);
LDNode *LDNodeLookup(LDNode *hash, const char *key);
void LDNodeFree(LDNode **hash);
unsigned int LDNodeCount(LDNode *hash);
/* functions for treating nodes as arrays */
LDNode *LDNodeCreateArray(void);
LDNode * LDNodeAppendBool(LDNode **array, bool b);
LDNode * LDNodeAppendNumber(LDNode **array, double n);
LDNode * LDNodeAppendString(LDNode **array, const char *s);
LDNode *LDNodeIndex(LDNode *array, unsigned int idx);

void LDSetLogFunction(int userlevel, void (userlogfn)(const char *));

/*
 * store interface. open files, read/write strings, ...
 */
typedef void *(*LD_store_opener)(void *, const char *, const char *, size_t);
typedef bool (*LD_store_stringwriter)(void *, const char *data);
typedef const char *(*LD_store_stringreader)(void *);
typedef void (*LD_store_closer)(void *);

void
LD_store_setfns(void *context, LD_store_opener, LD_store_stringwriter, LD_store_stringreader, LD_store_closer);

void *LD_store_fileopen(void *, const char *name, const char *mode, size_t len);
bool LD_store_filewrite(void *h, const char *data);
const char *LD_store_fileread(void *h);
void LD_store_fileclose(void *h);

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
bool LDClientRegisterFeatureFlagListener(struct LDClient_i *, const char *, LDlistenerfn);
bool LDClientUnregisterFeatureFlagListener(struct LDClient_i *, const char *, LDlistenerfn);

#ifdef __cplusplus
}


class LDClient {
    public:
        static LDClient *Get(void);
        static LDClient *Init(LDConfig *, LDUser *);

        bool isInitialized(void);

        bool boolVariation(const std::string &, bool);
        int intVariation(const std::string &, int);
        double doubleVariation(const std::string &, double);
        std::string stringVariation(const std::string &, const std::string &);
        char *stringVariation(const std::string &, const std::string &, char *, size_t);

        LDNode *JSONVariation(const std::string &, LDNode *);

        void setOffline();
        void setOnline();
        bool isOffline();

        std::string saveFlags();
        void restoreFlags(const std::string &);

        void flush(void);
        void close(void);
    private:
        struct LDClient_i *client;
};


#endif