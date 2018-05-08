
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
    LDNodeMap,
} LDNodeType;

typedef struct LDMapNode_i {
    char *key;
    LDNodeType type;
    union {
        bool b;
        char *s;
        double n;
        struct LDMapNode_i *m;
    };
    UT_hash_handle hh;
#ifdef __cplusplus
    struct LDMapNode_i *lookup(const std::string &key);
    void release(void);
#endif
} LDMapNode;

typedef struct LDConfig_i {
    bool allAttributesPrivate;
    int backgroundPollingIntervalMillis;
    char *appURI;
    int connectionTimeoutMillis;
    bool disableBackgroundUpdating;
    int eventsCapacity;
    int eventsFlushIntervalMillis;
    char *eventsURI;
    char *mobileKey;
    bool offline;
    int pollingIntervalMillis;
    LDMapNode *privateAttributeNames;
    bool streaming;
    char *streamURI;
    bool useReport;
} LDConfig;

typedef struct LDUser_i {
    char *key;
    bool anonymous;
    char *secondary;
    char *ip;
    char *firstName;
    char *lastName;
    char *email;
    char *name;
    char *avatar;
    LDMapNode *custom;
    LDMapNode *privateAttributeNames;
} LDUser;

struct LDClient_i;

#if !defined(__cplusplus) && !defined(LD_C_API)
typedef struct LDClient_i LDClient;
#endif


void LDSetString(char **, const char *);

LDConfig *LDConfigNew(const char *);
LDUser *LDUserNew(const char *);

struct LDClient_i *LDClientInit(LDConfig *, LDUser *);
struct LDClient_i *LDClientGet(void);

char *LDClientSaveFlags(struct LDClient_i *client);
void LDClientRestoreFlags(struct LDClient_i *client, const char *data);

void LDClientIdentify(struct LDClient_i *, LDUser *);

void LDClientFlush(struct LDClient_i *client);
bool LDClientIsInitialized(struct LDClient_i *);
bool LDClientIsOffline(struct LDClient_i *);
void LDClientSetOffline(struct LDClient_i *);
void LDClientSetOnline(struct LDClient_i *);
void LDClientClose(struct LDClient_i *);


bool LDBoolVariation(struct LDClient_i *, const char *, bool);
int LDIntVariation(struct LDClient_i *, const char *, int);
double LDDoubleVariation(struct LDClient_i *, const char *, double);
char *LDStringVariationAlloc(struct LDClient_i *, const char *, const char *);
char *LDStringVariation(struct LDClient_i *, const char *, const char *, char *, size_t);
LDMapNode *LDJSONVariation(struct LDClient_i *client, const char *key);
void LDJSONRelease(LDMapNode *m);

void LDFree(void *);
void *LDAlloc(size_t amt);

LDMapNode *LDMapLookup(LDMapNode *hash, const char *key);

void LD_SetLogFunction(int userlevel, void (userlogfn)(const char *));

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
bool LDClientRegisterListenerFunction(struct LDClient_i *, const char *, LDlistenerfn);
bool LDClientUnregisterListenerFunction(struct LDClient_i *, const char *, LDlistenerfn);

#ifdef __cplusplus
}


class LDClient {
    public:
        static LDClient *Get(void);
        static LDClient *Init(LDConfig *, LDUser *);

        bool isInitialized(void);

        bool boolVariation(const std::string &, bool);
        int intVariation(const std::string &, int);
        std::string stringVariation(const std::string &, const std::string &);
        char *stringVariation(const std::string &, const std::string &, char *, size_t);

        LDMapNode *JSONVariation(const std::string &);

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