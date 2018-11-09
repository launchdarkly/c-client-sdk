#ifndef C_CLIENT_LDINTERNAL_H
#define C_CLIENT_LDINTERNAL_H

#include "cJSON.h"
#ifndef _WINDOWS
#include <pthread.h>
#else
#include <windows.h>
#endif

struct listener {
    LDlistenerfn fn;
    char *key;
    struct listener *next;
};

struct LDClient_i {
    LDConfig *config;
    LDUser *user;
    LDNode *allFlags;
    bool offline;
    bool background;
    bool dead;
    bool isinit;
    struct listener *listeners;
};

struct LDConfig_i {
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
    LDNode *privateAttributeNames;
    bool streaming;
    char *streamURI;
    bool useReport;
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
    LDNode *custom;
    LDNode *privateAttributeNames;
};

struct IdentifyEvent {
    char *kind;
    char *key;
    double creationDate;
};

struct FeatureRequestEvent {
    char *kind;
    char *key;
    LDNode *value;
    LDNode Default;
};

unsigned char * LDi_base64_encode(const unsigned char *src, size_t len,
	size_t *out_len);
void LDi_freehash(LDNode *hash);
void LDi_freenode(LDNode *node);

void LDi_freeuser(LDUser *user);

char *LDi_hashtostring(LDNode *hash, bool versioned);
cJSON *LDi_hashtojson(LDNode *hash);
cJSON *LDi_arraytojson(LDNode *hash);
LDNode *LDi_jsontohash(cJSON *json, int flavor);
void LDi_initevents(int capacity);

char *LDi_strdup(const char *src);

bool LDi_clientsetflags(LDClient *client, bool needlock, const char *data, int flavor);
void LDi_savehash(LDClient *client);

void LDi_cancelread(int handle);
char *LDi_fetchfeaturemap(const char *urlprefix, const char *authkey, int *response,
    const char *const userjson, bool usereport);
void LDi_readstream(const char *urlprefix, const char *authkey, int *response, int cbdata(const char *),
    void cbhandle(int), const char *const userjson, bool usereport);

void LDi_recordidentify(LDClient *client, LDUser *lduser);
void LDi_recordfeature(LDClient *client, LDUser *lduser, LDNode *res, const char *feature, int type, double n, const char *s,
    LDNode *, double defaultn, const char *defaults, LDNode *);
void LDi_recordtrack(LDClient *client, LDUser *user, const char *name, LDNode *data);
char *LDi_geteventdata(void);
void LDi_sendevents(const char *url, const char *authkey, const char *eventdata, int *response);

void LDi_reinitializeconnection();
void LDi_startstopstreaming(bool stopstreaming);
void LDi_onstreameventput(const char *data);
void LDi_onstreameventpatch(const char *data);
void LDi_onstreameventdelete(const char *data);

char *LDi_usertojsontext(LDClient *client, LDUser *lduser, bool redact);
cJSON *LDi_usertojson(LDClient *client, LDUser *lduser, bool redact);

void LDi_log(int level, const char *fmt, ...);
void (*LDi_statuscallback)(int);

#define LD_ASSERT(condition) \
    if (!(condition)) { \
        LDi_log(0, "LD_ASSERT failed: expected condition '%s' in function '%s' aborting\n", #condition, __func__); \
        abort(); \
    } \

void LDi_millisleep(int ms);
/* must be called exactly once before rng is used */
void LDi_initializerng();
/* returns true on success, leaves result clean on failure*/
bool LDi_random(unsigned int *result);
/* returns true on success, may leave buffer dirty */
bool LDi_randomhex(char *buffer, size_t buffersize);
void LDi_startthreads(LDClient *client);

/* calls into the store interface */
void LDi_savedata(const char *dataname, const char *username, const char *data);
char *LDi_loaddata(const char *dataname, const char *username);

#ifndef _WINDOWS
#define ld_thread_t pthread_t
#define LDi_createthread(thread, fn, arg) pthread_create(thread, NULL, fn, arg)

#define ld_rwlock_t pthread_rwlock_t
#define LD_RWLOCK_INIT PTHREAD_RWLOCK_INITIALIZER
#define LDi_rdlock(lk) pthread_rwlock_rdlock(lk)
#define LDi_wrlock(lk) pthread_rwlock_wrlock(lk)
#define LDi_rdunlock(lk) pthread_rwlock_unlock(lk)
#define LDi_wrunlock(lk) pthread_rwlock_unlock(lk)

#define ld_mutex_t pthread_mutex_t
#define LDi_mtxinit(mtx) pthread_mutex_init(mtx, NULL)
#define LDi_mtxenter(mtx) pthread_mutex_lock(mtx)
#define LDi_mtxleave(mtx) pthread_mutex_unlock(mtx)

#define ld_cond_t pthread_cond_t
#define LD_COND_INIT PTHREAD_COND_INITIALIZER

#define ld_once_t pthread_once_t
#define LD_ONCE_INIT PTHREAD_ONCE_INIT
#define LDi_once(once, fn) pthread_once(once, fn)
/* windows */
#else
#define ld_thread_t HANDLE
void LDi_createthread(HANDLE *thread, LPTHREAD_START_ROUTINE fn, void *arg);
#define ld_rwlock_t SRWLOCK
#define LD_RWLOCK_INIT SRWLOCK_INIT
#define LDi_rdlock(lk) AcquireSRWLockShared(lk)
#define LDi_wrlock(lk) AcquireSRWLockExclusive(lk)
#define LDi_rdunlock(lk) ReleaseSRWLockShared(lk)
#define LDi_wrunlock(lk) ReleaseSRWLockExclusive(lk)

#define ld_mutex_t CRITICAL_SECTION
#define LDi_mtxinit(mtx) InitializeCriticalSection(mtx)
#define LDi_mtxenter(mtx) EnterCriticalSection(mtx)
#define LDi_mtxleave(mtx) LeaveCriticalSection(mtx)

#define ld_cond_t CONDITION_VARIABLE
#define LD_COND_INIT CONDITION_VARIABLE_INIT

#define ld_once_t INIT_ONCE
#define LD_ONCE_INIT INIT_ONCE_STATIC_INIT
void LDi_once(ld_once_t *once, void (*fn)(void));
#endif

void LDi_condwait(ld_cond_t *cond, ld_mutex_t *mtx, int ms);
void LDi_condsignal(ld_cond_t *cond);

extern ld_rwlock_t LDi_clientlock;
extern ld_cond_t LDi_bgeventcond;
extern ld_mutex_t LDi_allocmtx;
extern ld_once_t LDi_earlyonce;
void LDi_earlyinit(void);

/* expects caller to own LDi_clientlock */
void LDi_updatestatus(struct LDClient_i *client, bool isinit);

#endif /* C_CLIENT_LDINTERNAL_H */
