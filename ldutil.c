#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include "ldapi.h"
#include "ldinternal.h"

/*
 * set a string value, copying the memory, and freeing the old.
 */
void
LDSetString(char **target, const char *value)
{
    LDFree(*target);
    if (value) {
        *target = LDi_strdup(value);
    } else {
        *target = NULL;
    }
}

void
LDi_millisleep(int ms)
{
    ms += 500;
    sleep(ms / 1000);
}

LDMapNode *
LDMapLookup(LDMapNode *hash, const char *key)
{
    LDMapNode *res = NULL;

    LDMapNode *node, *tmp;

    HASH_FIND_STR(hash, key, res);
    return res;
}

static pthread_mutex_t allocmtx = PTHREAD_MUTEX_INITIALIZER;
unsigned long long LD_allocations;
unsigned long long LD_frees;

void *
LDAlloc(size_t amt)
{
    void *v = malloc(amt);
    if (v) {
        LDi_mtxenter(&allocmtx);
        LD_allocations++;
        LDi_mtxleave(&allocmtx);
    }
    return v;
}

void
LDFree(void *v)
{
    if (v) {
        LDi_mtxenter(&allocmtx);
        LD_frees++;
        LDi_mtxleave(&allocmtx);
        free(v);
    }
}


char *
LDi_strdup(const char *src)
{
    char *cp = strdup(src);
    if (cp) {
        LDi_mtxenter(&allocmtx);
        LD_allocations++;
        LDi_mtxleave(&allocmtx);
    }
    return cp;
}


/*
 * some functions to help with threads.
 * we're not really pthreads independent, but could be with some effort.
 */
void
LDi_condwait(pthread_cond_t *cond, pthread_mutex_t *mtx, int ms)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (ms % 1000) * 1000 * 1000;
    if (ts.tv_nsec > 1000 * 1000 * 1000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000 * 1000 * 1000;
    }

    int rv = pthread_cond_timedwait(cond, mtx, &ts);

}

void
LDi_condsignal(pthread_cond_t *cond)
{
    pthread_cond_signal(cond);
}

char *
LDi_usertourl(LDUser *user)
{
    cJSON *jsonuser = LDi_usertojson(user);
    char *textuser = cJSON_PrintUnformatted(jsonuser);
    cJSON_Delete(jsonuser);
    size_t b64len;
    char *b64text = LDi_base64_encode(textuser, strlen(textuser), &b64len);
    free(textuser);
    return b64text;
}

