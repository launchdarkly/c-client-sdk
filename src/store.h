#pragma once

#include <launchdarkly/api.h>

#include "concurrency.h"
#include "flag.h"
#include "reference_count.h"
#include "uthash.h"
#include "flag_change_listener.h"

struct LDStoreNode
{
    struct LDFlag  flag;
    struct ld_rc_t rc;
    UT_hash_handle hh;
};

struct LDStore
{
    struct LDStoreNode     *flags;
    struct ChangeListener  *listeners;
    LDBoolean               initialized;
    ld_rwlock_t             lock;
};

LDBoolean
LDi_storeInitialize(struct LDStore *const store);

void
LDi_storeDestroy(struct LDStore *const store);

LDBoolean
LDi_storeUpsert(struct LDStore *const store, struct LDFlag flag);

LDBoolean
LDi_storePut(
    struct LDStore *const store,
    struct LDFlag *       flags,
    const unsigned int    flagCount);

LDBoolean
LDi_storeDelete(
    struct LDStore *const store,
    const char *const     key,
    const unsigned int    version);

struct LDStoreNode *
LDi_storeGet(struct LDStore *const store, const char *const key);

LDBoolean
LDi_storeGetAll(
    struct LDStore *const       store,
    struct LDStoreNode ***const flags,
    unsigned int *const         flagCount);

struct LDJSON *
LDi_storeGetJSON(struct LDStore *const store);

LDBoolean
LDi_storeRegisterListener(
    struct LDStore *const store, const char *const flagKey, LDlistenerfn op);

void
LDi_storeUnregisterListener(
    struct LDStore *const store, const char *const flagKey, LDlistenerfn op);

void
LDi_storeFreeFlags(struct LDStore *const store);
