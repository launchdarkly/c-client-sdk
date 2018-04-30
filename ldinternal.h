
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
    LDStringMap *allFlags;
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

char *LDi_hashtojson(LDMapNode *hash);
LDMapNode *LDi_jsontohash(cJSON *json, int flavor);
void LDi_initevents(int capacity);
char * LDi_usertourl(LDUser *user);

void LDi_clientsetflags(LDClient *client, const char *data, int flavor);

char *LDi_fetchfeaturemap(const char *url, const char *authkey, int *response);
void LDi_readstream(const char *url, const char *authkey, int *response, int callback(const char *));

void LDi_recordidentify(LDUser *lduser);
void LDi_recordfeature(LDUser *lduser, const char *feature, int type, double n, const char *s,
    double defaultn, const char *defaults);
char *LDi_geteventdata(void);
void LDi_sendevents(const char *url, const char *authkey, const char *eventdata, int *response);

cJSON *LDi_usertojson(LDUser *lduser);

void LDi_log(int level, const char *fmt, ...);

void LDi_millisleep(int ms);
void LDi_startthreads(LDClient *client);

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
