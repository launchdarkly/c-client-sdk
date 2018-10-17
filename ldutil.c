#include <stdlib.h>
#include <stdio.h>
#ifndef _WINDOWS
#include <unistd.h>
#endif
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
#ifndef _WINDOWS
    ms += 500;
    sleep(ms / 1000);
#else
    Sleep(ms);
#endif
}

unsigned int
LDi_random(void)
{
#ifndef _WINDOWS
    return random();
#else
    return rand();
#endif
}

void
LDi_randomhex(char *const buffer, const size_t buffersize)
{
    const char *const alphabet = "0123456789ABCDEF";
    for (size_t i = 0; i < buffersize; i++) {
        buffer[i] = alphabet[LDi_random() % 16];
    }
}

ld_mutex_t LDi_allocmtx;
unsigned long long LD_allocations;
unsigned long long LD_frees;

void *
LDAlloc(size_t amt)
{
    void *v = malloc(amt);
    if (v) {
        LDi_mtxenter(&LDi_allocmtx);
        LD_allocations++;
        LDi_mtxleave(&LDi_allocmtx);
    }
    return v;
}

void
LDFree(void *v)
{
    if (v) {
        LDi_mtxenter(&LDi_allocmtx);
        LD_frees++;
        LDi_mtxleave(&LDi_allocmtx);
        free(v);
    }
}


char *
LDi_strdup(const char *src)
{
    char *cp = strdup(src);
    if (cp) {
        LDi_mtxenter(&LDi_allocmtx);
        LD_allocations++;
        LDi_mtxleave(&LDi_allocmtx);
    }
    return cp;
}


/*
 * some functions to help with threads.
 */
#ifndef _WINDOWS
/* posix */
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
    pthread_cond_broadcast(cond);
}
#else
/* windows */
void
LDi_condwait(CONDITION_VARIABLE *cond, CRITICAL_SECTION *mtx, int ms)
{
    SleepConditionVariableCS(cond, mtx, ms);

}

void
LDi_condsignal(CONDITION_VARIABLE *cond)
{
    WakeAllConditionVariable(cond);
}

void
LDi_createthread(HANDLE *thread, LPTHREAD_START_ROUTINE fn, void *arg)
{
    DWORD id;
    *thread = CreateThread(NULL, 0, fn, arg, 0, &id);
}

static BOOL CALLBACK
OneTimeCaller(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *lpContext)
{
    void (*fn)(void) = Parameter;

    fn();
    return TRUE;
}

void LDi_once(ld_once_t *once, void (*fn)(void))
{
    InitOnceExecuteOnce(once, OneTimeCaller, fn, NULL);
}
#endif

char *
LDi_usertojsontext(LDUser *user)
{
    cJSON *const jsonuser = LDi_usertojson(user);

    if (!jsonuser) {
        LDi_log(2, "LDi_usertojson failed in LDi_usertojsontext\n");
        return NULL;
    }

    char *const textuser = cJSON_PrintUnformatted(jsonuser);
    cJSON_Delete(jsonuser);

    if (!textuser) {
        LDi_log(2, "cJSON_PrintUnformatted failed in LDi_usertojsontext\n");
        return NULL;
    }

    return textuser;
}
