#include <launchdarkly/memory.h>

#include "assertion.h"
#include "store.h"
#include "uthash.h"

static void
LDi_destroyStoreNode(void *const nodeRaw)
{
    struct LDStoreNode *node;

    node = (struct LDStoreNode *)nodeRaw;

    if (node) {
        LDi_rc_destroy(&node->rc);
        LDi_flag_destroy(&node->flag);
        LDFree(nodeRaw);
    }
}

static void
LDi_storeFreeHash(struct LDStoreNode *flags)
{
    struct LDStoreNode *node, *tmp;

    HASH_ITER(hh, flags, node, tmp)
    {
        HASH_DEL(flags, node);

        LDi_destroyStoreNode(node);
    }
}

void
LDi_storeFreeFlags(struct LDStore *const store)
{
    LD_ASSERT(store);

    LDi_storeFreeHash(store->flags);

    store->flags = NULL;
}

LDBoolean
LDi_storeInitialize(struct LDStore *const store)
{
    LD_ASSERT(store);

    if (!LDi_rwlock_init(&store->lock)) {
        return LDBooleanFalse;
    }

    store->flags       = NULL;
    store->initialized = LDBooleanFalse;

    LDi_initListeners(&store->listeners);

    return LDBooleanTrue;
}

void
LDi_storeDestroy(struct LDStore *const store)
{
    if (store) {
        LDi_storeFreeHash(store->flags);
        LDi_rwlock_destroy(&store->lock);
        LDi_freeListeners(&store->listeners);
    }
}

static struct LDStoreNode *
LDi_allocateStoreNode(struct LDFlag flag)
{
    struct LDStoreNode *node;

    if (!(node = LDAlloc(sizeof(struct LDStoreNode)))) {
        return NULL;
    }

    if (!LDi_rc_initialize(&node->rc, (void *)node, LDi_destroyStoreNode)) {
        LDFree(node);

        return NULL;
    }

    node->flag = flag;

    return node;
}

static void
LDi_fireListenersFor(
    struct LDStore *const store, const char *const key, const LDBoolean deleted)
{
    LD_ASSERT(store);
    LD_ASSERT(key);

    LDi_listenersDispatch(store->listeners, key, deleted);
}

enum versionStatus {
    /* This version represents a new flag that didn't exist before. */
    VERSION_NEW,
    /* Received an update to an existing flag. */
    VERSION_INCREASED,
    /* Received a stale update to an existing flag. */
    VERSION_STALE
};

/* Computes the meaning of an incoming flag version, when compared
 * to the existing store node for that flag. */
static enum versionStatus
versionStatus(struct LDStoreNode *existingFlag, int incomingVersion)
{
    if (!existingFlag) {
        return VERSION_NEW;
    }
    if (incomingVersion > existingFlag->flag.version) {
        return VERSION_INCREASED;
    }
    return VERSION_STALE;
}


LDBoolean
LDi_storeUpsert(struct LDStore *const store, struct LDFlag flag)
{
    struct LDStoreNode *existing, *replacement;
    enum versionStatus status;

    LD_ASSERT(store);
    LD_ASSERT(flag.key);

    /* Theoretically reduce lock contention by eagerly allocating the replacement store node.
     * Downside: the allocation is unnecessary if the update is stale, but this is unlikely. */
    if (!(replacement = LDi_allocateStoreNode(flag))) {
        LDi_flag_destroy(&flag);

        return LDBooleanFalse;
    }

    LDi_rwlock_wrlock(&store->lock);

    HASH_FIND_STR(store->flags, flag.key, existing);

    status = versionStatus(existing, flag.version);

    switch (status) {
        case VERSION_NEW: {
            HASH_ADD_KEYPTR(hh, store->flags, flag.key, strlen(flag.key), replacement);
            break;
        }
        case VERSION_INCREASED: {
            HASH_DEL(store->flags, existing);
            LDi_rc_decrement(&existing->rc);
            HASH_ADD_KEYPTR(hh, store->flags, flag.key, strlen(flag.key), replacement);
            break;
        }
        case VERSION_STALE: {
            LDi_destroyStoreNode(replacement);
            break;
        }
    }

    if (status != VERSION_STALE) {
        LDi_fireListenersFor(store, flag.key, flag.deleted);
    }

    LDi_rwlock_wrunlock(&store->lock);

    return LDBooleanTrue;
}

struct LDStoreNode *
LDi_storeGet(struct LDStore *const store, const char *const key)
{
    struct LDStoreNode *lookup;

    LD_ASSERT(store);
    LD_ASSERT(key);

    LDi_rwlock_rdlock(&store->lock);

    HASH_FIND_STR(store->flags, key, lookup);

    if (lookup && !lookup->flag.deleted) {
        LDi_rc_increment(&lookup->rc);

        LDi_rwlock_rdunlock(&store->lock);

        return lookup;
    } else {
        LDi_rwlock_rdunlock(&store->lock);

        return NULL;
    }
}

LDBoolean
LDi_storeDelete(
    struct LDStore *const store,
    const char *const     key,
    const unsigned int    version)
{
    struct LDFlag flag;

    LD_ASSERT(store);
    LD_ASSERT(key);

    if (!(flag.key = LDStrDup(key))) {
        return LDBooleanFalse;
    }

    flag.value                = NULL;
    flag.version              = version;
    flag.variation            = 0;
    flag.trackEvents          = LDBooleanFalse;
    flag.trackReason          = LDBooleanFalse;
    flag.reason               = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted              = LDBooleanTrue;

    return LDi_storeUpsert(store, flag);
}

LDBoolean
LDi_storePut(
    struct LDStore *const store,
    struct LDFlag *       flags,
    const unsigned int    flagCount)
{
    size_t              i;
    LDBoolean           failed;
    struct LDStoreNode *flagsHash, *oldHash;

    LD_ASSERT(store);

    failed    = LDBooleanFalse;
    flagsHash = NULL;

    for (i = 0; i < flagCount; i++) {
        if (failed) {
            LDi_flag_destroy(&flags[i]);
        } else {
            struct LDStoreNode *node;

            if (!(node = LDi_allocateStoreNode(flags[i]))) {
                LD_LOG(LD_LOG_ERROR, "failed to allocate storage node for flag");

                failed = LDBooleanTrue;

                continue;
            }

            HASH_ADD_KEYPTR(
                hh, flagsHash, node->flag.key, strlen(node->flag.key), node);
        }
    }

    LDFree(flags);

    if (failed) {
        LDi_storeFreeHash(flagsHash);
    } else {
        struct LDStoreNode *node, *tmp;

        LDi_rwlock_wrlock(&store->lock);

        oldHash            = store->flags;
        store->flags       = flagsHash;
        store->initialized = LDBooleanTrue;

        HASH_ITER(hh, store->flags, node, tmp)
        {
            LDi_fireListenersFor(store, node->flag.key, LDBooleanFalse);
        }

        LDi_rwlock_wrunlock(&store->lock);

        LDi_storeFreeHash(oldHash);
    }

    return !failed;
}

LDBoolean
LDi_storeGetAll(
    struct LDStore *const       store,
    struct LDStoreNode ***const flags,
    unsigned int *const         flagCount)
{
    unsigned int        count;
    struct LDStoreNode *node, *tmp, **dupe, **iter;

    LD_ASSERT(store);
    LD_ASSERT(flags);
    LD_ASSERT(flagCount);

    LDi_rwlock_rdlock(&store->lock);

    count = HASH_COUNT(store->flags);

    if (count == 0) {
        LDi_rwlock_rdunlock(&store->lock);
        *flags = NULL;
        *flagCount = 0;
        return LDBooleanTrue;
    }

    if (!(dupe = LDAlloc(sizeof(struct LDStoreNode *) * count))) {
        LDi_rwlock_rdunlock(&store->lock);

        return LDBooleanFalse;
    }

    iter = dupe;

    HASH_ITER(hh, store->flags, node, tmp)
    {
        *iter = node;
        LDi_rc_increment(&node->rc);
        iter++;
    }

    LDi_rwlock_rdunlock(&store->lock);

    *flags     = dupe;
    *flagCount = count;

    return LDBooleanTrue;
}

struct LDJSON *
LDi_storeGetJSON(struct LDStore *const store)
{
    struct LDJSON *     result, *flag;
    struct LDStoreNode *node, *tmp;

    result = NULL;
    flag   = NULL;
    node   = NULL;
    tmp    = NULL;

    LD_ASSERT(store);

    if (!(result = LDNewObject())) {
        return NULL;
    }

    LDi_rwlock_rdlock(&store->lock);

    HASH_ITER(hh, store->flags, node, tmp)
    {

        if (node->flag.deleted) {
            continue;
        }

        if (!(flag = LDi_flag_to_json(&node->flag))) {
            goto error;
        }

        if (!LDObjectSetKey(result, node->flag.key, flag)) {
            goto error;
        }
        flag = NULL;
    }

    LDi_rwlock_rdunlock(&store->lock);

    return result;

error:
    LDJSONFree(result);
    LDJSONFree(flag);

    LDi_rwlock_rdunlock(&store->lock);

    return NULL;
}

/* Registers a listener callback for a given flag, returning true on success or if the combination of flag key and listener
 * callback is already registered. */
LDBoolean
LDi_storeRegisterListener(struct LDStore *const store, const char *const flagKey, LDlistenerfn op)
{
    LDBoolean status;

    LD_ASSERT(store);
    LD_ASSERT(flagKey);
    LD_ASSERT(op);

    LDi_rwlock_wrlock(&store->lock);
    status = LDi_listenerAdd(&store->listeners, flagKey, op);
    LDi_rwlock_wrunlock(&store->lock);

    return status;
}

void
LDi_storeUnregisterListener(struct LDStore *const store, const char *const flagKey, LDlistenerfn op)
{

    LD_ASSERT(store);
    LD_ASSERT(flagKey);
    LD_ASSERT(op);

    LDi_rwlock_wrlock(&store->lock);
    LDi_listenerRemove(&store->listeners, flagKey, op);
    LDi_rwlock_wrunlock(&store->lock);
}
