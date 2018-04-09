
#include "cJSON.h"
#include <pthread.h>

struct LDClient_i {
    LDConfig *config;
    LDUser *user;
    LDStringMap *allFlags;
    bool offline;
    bool dead;
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

LDMapNode *LDi_jsontohash(cJSON *json, int flavor);
void LDi_initevents(int capacity);
char * LDi_usertourl(LDUser *user);

LDMapNode *LDi_fetchfeaturemap(const char *url, const char *authkey, int *response);
void LDi_readstream(const char *url, const char *authkey, int *response, int callback(const char *));

void LDi_recordidentify(LDUser *lduser);
void LDi_recordfeature(LDUser *lduser, const char *feature, int type, double n, const char *s,
    double defaultn, const char *defaults);
char *LDi_geteventdata(void);
void LDi_sendevents(const char *url, const char *authkey, const char *eventdata, int *response);

cJSON *LDi_usertojson(LDUser *lduser);

void LDi_log(int level, const char *fmt, ...);

#if 0
#define LDi_rdlock(lk) do { *(lk) = 1; } while (0)
#define LDi_wrlock(lk) do { *(lk) = 1; } while (0)
#define LDi_unlock(lk) do { *(lk) = 0; } while (0)
#else
#define LDi_rdlock(lk) pthread_rwlock_rdlock(lk)
#define LDi_wrlock(lk) pthread_rwlock_wrlock(lk)
#define LDi_unlock(lk) pthread_rwlock_unlock(lk)
#endif

#define LDi_mtxenter(mtx)
#define LDi_mtxleave(mtx)

