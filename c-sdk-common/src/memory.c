#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <curl/curl.h>

#include "cJSON.h"

#include <launchdarkly/memory.h>

#include "assertion.h"

static void *(*customAlloc)(const size_t bytes) = malloc;

void *
LDAlloc(const size_t bytes)
{
    return customAlloc(bytes);
}

static void (*customFree)(void *const buffer) = free;

void
LDFree(void *const buffer)
{
    customFree(buffer);
}

static char *(*customStrDup)(const char *const string) = strdup;

char *
LDStrDup(const char *const string)
{
    return customStrDup(string);
}

static void *(*customRealloc)(void *const buffer, const size_t bytes) = realloc;

void *
LDRealloc(void *const buffer, const size_t bytes)
{
    return customRealloc(buffer, bytes);
}

static void *(*customCalloc)(const size_t nmemb, const size_t size) = calloc;

void *
LDCalloc(const size_t nmemb, const size_t size)
{
    return customCalloc(nmemb, size);
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

static char *(*customStrNDup)(const char *const str, const size_t n) =
    LDi_strndup;

char *
LDStrNDup(const char *const str, const size_t n)
{
    return customStrNDup(str, n);
}

void
LDSetMemoryRoutines(void *(*const newMalloc)(const size_t),
    void (*const newFree)(void *const),
    void *(*const newRealloc)(void *const, const size_t),
    char *(*const newStrDup)(const char *const),
    void *(*const newCalloc)(const size_t, const size_t),
    char *(*const newStrNDup)(const char *const, const size_t))
{
    LD_ASSERT(newMalloc);
    LD_ASSERT(newFree);
    LD_ASSERT(newRealloc);
    LD_ASSERT(newStrDup);
    LD_ASSERT(newCalloc);

    customAlloc   = newMalloc;
    customFree    = newFree;
    customRealloc = newRealloc;
    customStrDup  = newStrDup;
    customCalloc  = newCalloc;
    customStrNDup = newStrNDup;
}

void
LDGlobalInit(void)
{
    static bool first = true;

    if (first) {
        struct cJSON_Hooks hooks;
        CURLcode status;

        first = false;

        status = curl_global_init_mem(CURL_GLOBAL_DEFAULT, LDAlloc, LDFree,
            LDRealloc, LDStrDup, LDCalloc);
        LD_ASSERT(!status);

        hooks.malloc_fn = LDAlloc;
        hooks.free_fn   = LDFree;

        cJSON_InitHooks(&hooks);
    }
}
