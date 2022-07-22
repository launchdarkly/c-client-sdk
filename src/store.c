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

LDBoolean
LDi_storeUpsert(struct LDStore *const store, struct LDFlag flag)
{
    struct LDStoreNode *existing, *replacement;

    LD_ASSERT(store);
    LD_ASSERT(flag.key);

    /* Allocate before lock even though it may be throw away to reduce lock
    contention as old flags should be rare */
    if (!(replacement = LDi_allocateStoreNode(flag))) {
        LDi_flag_destroy(&flag);

        return LDBooleanFalse;
    }

    LDi_rwlock_wrlock(&store->lock);

    HASH_FIND_STR(store->flags, flag.key, existing);

    if ((existing && (flag.version >= existing->flag.version)) || !existing) {
        if (existing) {
            HASH_DEL(store->flags, existing);
            LDi_rc_decrement(&existing->rc);
        }

        HASH_ADD_KEYPTR(
            hh, store->flags, flag.key, strlen(flag.key), replacement);

        LDi_fireListenersFor(store, flag.key, flag.deleted);
    } else {
        LDi_rc_destroy(&replacement->rc);
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

/* Registers a listener callback for a given flag, returning true on success or if the combination of flag key and listener
 * callback is already registered. */
LDBoolean
LDi_storeRegisterListenerUserData(struct LDStore *const store, const char *const flagKey, LDlistenerUserDatafn op, void *const userData)
{
    LDBoolean status;

    LD_ASSERT(store);
    LD_ASSERT(flagKey);
    LD_ASSERT(op);

    LDi_rwlock_wrlock(&store->lock);
    status = LDi_listenerUserDataAdd(&store->listeners, flagKey, op, userData);
    LDi_rwlock_wrunlock(&store->lock);

    return status;
}

void
LDi_storeUnregisterListenerUserData(struct LDStore *const store, const char *const flagKey, LDlistenerUserDatafn op)
{

    LD_ASSERT(store);
    LD_ASSERT(flagKey);
    LD_ASSERT(op);

    LDi_rwlock_wrlock(&store->lock);
    LDi_listenerUserDataRemove(&store->listeners, flagKey, op);
    LDi_rwlock_wrunlock(&store->lock);
}
