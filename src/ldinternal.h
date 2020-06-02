#ifndef C_CLIENT_LDINTERNAL_H
#define C_CLIENT_LDINTERNAL_H

#include "cJSON.h"
#ifndef _WINDOWS
#include <pthread.h>
#else
#include <windows.h>
#endif

#include "concurrency.h"
#include "utility.h"
#include "sse.h"
#include "event_processor.h"
#include "store.h"

#ifndef _WINDOWS

#define ld_once_t pthread_once_t
#define LD_ONCE_INIT PTHREAD_ONCE_INIT
#define LDi_once(once, fn) pthread_once(once, fn)
/* windows */
#else

#define ld_once_t INIT_ONCE
#define LD_ONCE_INIT INIT_ONCE_STATIC_INIT
void LDi_once(ld_once_t *once, void (*fn)(void));
#endif

struct listener {
    LDlistenerfn fn;
    char *key;
    struct listener *next;
};

struct LDClient_i {
    struct LDGlobal_i *shared;
    char *mobileKey;
    ld_rwlock_t clientLock;
    bool offline;
    bool background;
    LDStatus status;
    struct listener *listeners;
    /* thread management */
    ld_thread_t eventThread;
    ld_thread_t pollingThread;
    ld_thread_t streamingThread;
    ld_cond_t eventCond;
    ld_cond_t pollCond;
    ld_cond_t streamCond;
    ld_mutex_t condMtx;
    /* streaming state */
    bool shouldstopstreaming;
    int streamhandle;
    struct EventProcessor *eventProcessor;
    struct LDStore store;
    /* init cond */
    ld_cond_t initCond;
    ld_mutex_t initCondMtx;
    UT_hash_handle hh;
};

struct LDConfig_i {
    bool allAttributesPrivate;
    int backgroundPollingIntervalMillis;
    char *appURI;
    int connectionTimeoutMillis;
    bool disableBackgroundUpdating;
    unsigned int eventsCapacity;
    int eventsFlushIntervalMillis;
    char *eventsURI;
    char *mobileKey;
    bool offline;
    int pollingIntervalMillis;
    bool streaming;
    char *streamURI;
    bool useReport;
    char *proxyURI;
    bool verifyPeer;
    bool useReasons;
    char *certFile;
    bool inlineUsersInEvents;
    /* map of name -> key */
    struct LDJSON *secondaryMobileKeys;
    /* array of strings */
    struct LDJSON *privateAttributeNames;
};

struct LDUser_i {
    char *key;
    bool anonymous;
    char *secondary;
    char *ip;
    char *firstName;
    char *lastName;
    char *email;
    char *name;
    char *avatar;
    struct LDJSON *privateAttributeNames;
    struct LDJSON *custom;
};

struct LDGlobal_i {
    LDClient *clientTable;
    LDClient *primaryClient;
    LDConfig *sharedConfig;
    LDUser *sharedUser;
    ld_rwlock_t sharedUserLock;
};

LDClient *LDi_clientinitisolated(struct LDGlobal_i *shared,
    const char *mobileKey);

unsigned char * LDi_base64_encode(const unsigned char *src, size_t len,
	size_t *out_len);

void LDi_cancelread(int handle);
char *LDi_fetchfeaturemap(LDClient *client, int *response);

void LDi_readstream(LDClient *const client, int *response,
    struct LDSSEParser *const parser,
    void cbhandle(LDClient *client, int handle));

void LDi_sendevents(LDClient *client, const char *eventdata,
    const char *const payloadUUID, int *response);

void LDi_reinitializeconnection(LDClient *client);
void LDi_startstopstreaming(LDClient *client, bool stopstreaming);
void LDi_onstreameventput(LDClient *client, const char *data);
void LDi_onstreameventpatch(LDClient *client, const char *data);
void LDi_onstreameventdelete(LDClient *client, const char *data);

char *LDi_usertojsontext(const LDClient *const client,
    const LDUser *const lduser, bool redact);

struct LDJSON *
LDi_userToJSON(
    const LDConfig *const config,
    const LDUser *const   lduser,
    const bool            redact
);

void (*LDi_statuscallback)(int);

#define LD_ASSERT(condition) \
    if (!(condition)) { \
        LDi_log(0, "LD_ASSERT failed: expected condition '%s' in function '%s' aborting\n", #condition, __func__); \
        abort(); \
    } \

void LDi_millisleep(int ms);
/* must be called exactly once before rng is used */
void LDi_initializerng();
/* returns true on success, may leave buffer dirty */
bool LDi_randomhex(char *buffer, size_t buffersize);
/* returns a unique device identifier, returns NULL on failure */
char *LDi_deviceid();

/* calls into the store interface */
void LDi_savedata(const char *dataname, const char *username, const char *data);
char *LDi_loaddata(const char *dataname, const char *username);

extern ld_mutex_t LDi_allocmtx;
extern ld_once_t LDi_earlyonce;
void LDi_earlyinit(void);

/* expects caller to own LDi_clientlock */
void LDi_updatestatus(struct LDClient_i *client, const LDStatus status);

THREAD_RETURN LDi_bgeventsender(void *const v);
THREAD_RETURN LDi_bgfeaturepoller(void *const v);
THREAD_RETURN LDi_bgfeaturestreamer(void *const v);

#endif /* C_CLIENT_LDINTERNAL_H */
