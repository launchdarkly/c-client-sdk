#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "cJSON.h"

#include <launchdarkly/boolean.h>
#include <launchdarkly/memory.h>

#include "assertion.h"
#include "memory.h"

void *(*LDi_customAlloc)(const size_t bytes) = malloc;

void *
LDAlloc(const size_t bytes)
{
    return LDi_customAlloc(bytes);
}

void (*LDi_customFree)(void *const buffer) = free;

void
LDFree(void *const buffer)
{
    LDi_customFree(buffer);
}

static char *
LDi_strdup(const char *const string)
{
#ifdef _WIN32
    return _strdup(string);
#else
    return strdup(string);
#endif
}

char *(*LDi_customStrDup)(const char *const string) = LDi_strdup;

char *
LDStrDup(const char *const string)
{
    return LDi_customStrDup(string);
}

void *(*LDi_customRealloc)(void *const buffer, const size_t bytes) = realloc;

void *
LDRealloc(void *const buffer, const size_t bytes)
{
    return LDi_customRealloc(buffer, bytes);
}

void *(*LDi_customCalloc)(const size_t nmemb, const size_t size) = calloc;

void *
LDCalloc(const size_t nmemb, const size_t size)
{
    return LDi_customCalloc(nmemb, size);
}

static char *
LDi_strndup(const char *const str, const size_t n)
{
    char *result;

    if ((result = (char *)LDAlloc(n + 1))) {
        memcpy(result, str, n);
        result[n] = '\0';
        return result;
    } else {
        return NULL;
    }
}

char *(*LDi_customStrNDup)(const char *const str, const size_t n) = LDi_strndup;

char *
LDStrNDup(const char *const str, const size_t n)
{
    return LDi_customStrNDup(str, n);
}

void
LDSetMemoryRoutines(
    void *(*const newMalloc)(const size_t),
    void (*const newFree)(void *const),
    void *(*const newRealloc)(void *const, const size_t),
    char *(*const newStrDup)(const char *const),
    void *(*const newCalloc)(const size_t, const size_t),
    char *(*const newStrNDup)(const char *const, const size_t))
{
    LD_ASSERT_API(newMalloc);
    LD_ASSERT_API(newFree);
    LD_ASSERT_API(newRealloc);
    LD_ASSERT_API(newStrDup);
    LD_ASSERT_API(newCalloc);

    LDi_customAlloc   = newMalloc;
    LDi_customFree    = newFree;
    LDi_customRealloc = newRealloc;
    LDi_customStrDup  = newStrDup;
    LDi_customCalloc  = newCalloc;
    LDi_customStrNDup = newStrNDup;
}

void
LDGlobalInit(void)
{
    static LDBoolean first = LDBooleanTrue;

    if (first) {
        struct cJSON_Hooks hooks;
        CURLcode           status;

        first = LDBooleanFalse;

        status = curl_global_init_mem(
            CURL_GLOBAL_DEFAULT,
            LDAlloc,
            LDFree,
            LDRealloc,
            LDStrDup,
            LDCalloc);

        LD_ASSERT(!status);

        hooks.malloc_fn = LDAlloc;
        hooks.free_fn   = LDFree;

        cJSON_InitHooks(&hooks);
    }
}
