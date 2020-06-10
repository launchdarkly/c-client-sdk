#pragma once

#include <stdbool.h>

#include <launchdarkly/api.h>

#include "concurrency.h"
#include "flag.h"
#include "uthash.h"
#include "reference_count.h"

struct LDStoreListener {
    LDlistenerfn fn;
    char *key;
    struct LDStoreListener *next;
};

struct LDStoreNode {
    struct LDFlag flag;
    struct ld_rc_t rc;
    UT_hash_handle hh;
};

struct LDStore {
    struct LDStoreNode *flags;
    struct LDStoreListener *listeners;
    bool initialized;
    ld_rwlock_t lock;
};

bool LDi_storeInitialize(struct LDStore *const store);

void LDi_storeDestroy(struct LDStore *const store);

bool LDi_storeUpsert(struct LDStore *const store, struct LDFlag flag);

bool LDi_storePut(struct LDStore *const store,
    struct LDFlag *flags, const unsigned int flagCount);

bool LDi_storeDelete(struct LDStore *const store, const char *const key,
    const unsigned int version);

struct LDStoreNode *LDi_storeGet(struct LDStore *const store,
    const char *const key);

bool LDi_storeGetAll(struct LDStore *const store,
    struct LDStoreNode ***const flags, unsigned int *const flagCount);

struct LDJSON *LDi_storeGetJSON(struct LDStore *const store);

bool LDi_storeRegisterListener(struct LDStore *const store,
    const char *const flagKey, LDlistenerfn op);

void LDi_storeUnregisterListener(struct LDStore *const store,
    const char *const flagKey, LDlistenerfn op);

void LDi_storeFreeFlags(struct LDStore *const store);
