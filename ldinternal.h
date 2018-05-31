
#include "cJSON.h"
#include <pthread.h>

struct listener {
    LDlistenerfn fn;
    char *key;
    struct listener *next;
};

struct LDClient_i {
    LDConfig *config;
    LDUser *user;
    LDMapNode *allFlags;
    bool offline;
    bool dead;
    bool isinit;
    struct listener *listeners;
};

struct IdentifyEvent {
    char *kind;
    char *key;
    double creationDate;
};

struct FeatureRequestEvent {
    char *kind;
    char *key;
    LDMapNode *value;
    LDMapNode Default;
};

unsigned char * LDi_base64_encode(const unsigned char *src, size_t len,
	size_t *out_len);
void LDi_freehash(LDMapNode *hash);
void LDi_freenode(LDMapNode *node);

void LDi_freeuser(LDUser *user);

char *LDi_hashtostring(LDMapNode *hash);
cJSON *LDi_hashtojson(LDMapNode *hash);
cJSON *LDi_arraytojson(LDMapNode *hash);
LDMapNode *LDi_jsontohash(cJSON *json, int flavor);
void LDi_initevents(int capacity);
char * LDi_usertourl(LDUser *user);

char *LDi_strdup(const char *src);

bool LDi_clientsetflags(LDClient *client, bool needlock, const char *data, int flavor);
void LDi_savehash(LDClient *client);

char *LDi_fetchfeaturemap(const char *url, const char *authkey, int *response);
void LDi_readstream(const char *url, const char *authkey, int *response, int callback(const char *));

void LDi_recordidentify(LDUser *lduser);
void LDi_recordfeature(LDUser *lduser, const char *feature, int type, double n, const char *s,
    LDMapNode *, double defaultn, const char *defaults, LDMapNode *);
char *LDi_geteventdata(void);
void LDi_sendevents(const char *url, const char *authkey, const char *eventdata, int *response);

void LDi_onstreameventput(const char *data);
void LDi_onstreameventpatch(const char *data);
void LDi_onstreameventdelete(const char *data);

cJSON *LDi_usertojson(LDUser *lduser);

void LDi_log(int level, const char *fmt, ...);
void (*LDi_statuscallback)(int);

void LDi_millisleep(int ms);
void LDi_startthreads(LDClient *client);

/* calls into the store interface */
void LDi_savedata(const char *dataname, const char *username, const char *data);
char *LDi_loaddata(const char *dataname, const char *username);

#if 0
#define LDi_rdlock(lk) do { *(lk) = 1; } while (0)
#define LDi_wrlock(lk) do { *(lk) = 1; } while (0)
#define LDi_unlock(lk) do { *(lk) = 0; } while (0)
#else
#define LDi_rdlock(lk) pthread_rwlock_rdlock(lk)
#define LDi_wrlock(lk) pthread_rwlock_wrlock(lk)
#define LDi_unlock(lk) pthread_rwlock_unlock(lk)
void LDi_condwait(pthread_cond_t *cond, pthread_mutex_t *mtx, int ms);
void LDi_condsignal(pthread_cond_t *cond);
#endif

#define LDi_mtxenter(mtx) pthread_mutex_lock(mtx)
#define LDi_mtxleave(mtx) pthread_mutex_unlock(mtx)

extern pthread_rwlock_t LDi_clientlock;
extern pthread_cond_t LDi_bgeventcond;
