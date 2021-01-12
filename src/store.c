#include <launchdarkly/memory.h>

#include "assertion.h"
#include "uthash.h"
#include "store.h"

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

void
LDi_storeFreeHash(struct LDStoreNode *flags)
{
    struct LDStoreNode *node, *tmp;

    HASH_ITER(hh, flags, node, tmp) {
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

bool
LDi_storeInitialize(struct LDStore *const store)
{
    LD_ASSERT(store);

    if (!LDi_rwlock_init(&store->lock)) {
        return false;
    }

    store->flags       = NULL;
    store->listeners   = NULL;
    store->initialized = false;

    return true;
}

void
LDi_storeDestroy(struct LDStore *const store)
{
    if (store) {
        struct LDStoreListener *iter;

        LDi_storeFreeHash(store->flags);
        LDi_rwlock_destroy(&store->lock);

        iter = store->listeners;

        while (iter) {
            /* must record next to make delete safe */
            struct LDStoreListener *const next = iter->next;
            LDFree(iter->key);
            LDFree(iter);
            iter = next;
        }
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
LDi_fireListenersFor(struct LDStore *const store, const char *const key,
    const bool deleted)
{
    struct LDStoreListener *iter;

    LD_ASSERT(store);
    LD_ASSERT(key);

    for (iter = store->listeners; iter; iter = iter->next) {
        if (strcmp(key, iter->key) == 0) {
            iter->fn(key, deleted);
        }
    }
}

bool
LDi_storeUpsert(struct LDStore *const store, struct LDFlag flag)
{
    struct LDStoreNode *existing, *replacement;

    LD_ASSERT(store);
    LD_ASSERT(flag.key);

    /* Allocate before lock even though it may be throw away to reduce lock
    contention as old flags should be rare */
    if (!(replacement = LDi_allocateStoreNode(flag))) {
        LDi_flag_destroy(&flag);

        return false;
    }

    LDi_rwlock_wrlock(&store->lock);

    HASH_FIND_STR(store->flags, flag.key, existing);

    if ((existing && (flag.version >= existing->flag.version)) || !existing) {
        if (existing) {
            HASH_DEL(store->flags, existing);
            LDi_rc_decrement(&existing->rc);
        }

        HASH_ADD_KEYPTR(hh, store->flags, flag.key, strlen(flag.key),
            replacement);

        LDi_fireListenersFor(store, flag.key, flag.deleted);
    } else {
        LDi_rc_destroy(&replacement->rc);
    }

    LDi_rwlock_wrunlock(&store->lock);

    return true;
}

struct LDStoreNode *
LDi_storeGet(struct LDStore *const store,
    const char *const key)
{
    struct LDStoreNode *lookup;

    LD_ASSERT(store);
    LD_ASSERT(key);

    LDi_rwlock_rdlock(&store->lock);
    HASH_FIND_STR(store->flags, key, lookup);
    LDi_rwlock_rdunlock(&store->lock);

    if (lookup && !lookup->flag.deleted) {
        LDi_rc_increment(&lookup->rc);

        return lookup;
    } else {
        return NULL;
    }
}

bool
LDi_storeDelete(struct LDStore *const store, const char *const key,
    const unsigned int version)
{
    struct LDFlag flag;

    LD_ASSERT(store);
    LD_ASSERT(key);

    if (!(flag.key = LDStrDup(key))) {
        return false;
    }

    flag.value                = NULL;
    flag.version              = version;
    flag.variation            = 0;
    flag.trackEvents          = false;
    flag.reason               = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted              = true;

    return LDi_storeUpsert(store, flag);
}

bool
LDi_storePut(struct LDStore *const store, struct LDFlag *flags,
    const unsigned int flagCount)
{
    size_t i;
    bool failed;
    struct LDStoreNode *flagsHash, *oldHash;

    LD_ASSERT(store);

    failed    = false;
    flagsHash = NULL;

    for (i = 0; i < flagCount; i++) {
        if (failed) {
            LDi_flag_destroy(&flags[i]);
        } else {
            struct LDStoreNode *node;

            if (!(node = LDi_allocateStoreNode(flags[i]))) {
                failed = true;

                continue;
            }

            HASH_ADD_KEYPTR(hh, flagsHash, node->flag.key,
                strlen(node->flag.key), node);
        }
    }

    LDFree(flags);

    if (failed) {
        LDi_storeFreeHash(flagsHash);
    } else {
        struct LDStoreNode *node, *tmp;

        LDi_rwlock_wrlock(&store->lock);

        oldHash = store->flags;
        store->flags = flagsHash;
        store->initialized = true;

        HASH_ITER(hh, store->flags, node, tmp) {
            LDi_fireListenersFor(store, node->flag.key, false);
        }

        LDi_rwlock_wrunlock(&store->lock);

        LDi_storeFreeHash(oldHash);
    }

    return !failed;
}

bool
LDi_storeGetAll(struct LDStore *const store,
    struct LDStoreNode ***const flags, unsigned int *const flagCount)
{
    unsigned int count;
    struct LDStoreNode *node, *tmp, **dupe, **iter;

    LD_ASSERT(store);
    LD_ASSERT(flags);
    LD_ASSERT(flagCount);

    LDi_rwlock_rdlock(&store->lock);

    count = HASH_COUNT(store->flags);

    if (!(dupe = LDAlloc(sizeof(struct LDStoreNode *) * count))) {
        LDi_rwlock_rdunlock(&store->lock);

        return false;
    }

    iter = dupe;

    HASH_ITER(hh, store->flags, node, tmp) {
        *iter = node;
        LDi_rc_increment(&node->rc);
        iter++;
    }

    LDi_rwlock_rdunlock(&store->lock);

    *flags     = dupe;
    *flagCount = count;

    return true;
}

struct LDJSON *
LDi_storeGetJSON(struct LDStore *const store)
{
    struct LDJSON *result, *flag;
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

    HASH_ITER(hh, store->flags, node, tmp) {
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

bool
LDi_storeRegisterListener(struct LDStore *const store,
    const char *const flagKey, LDlistenerfn op)
{
    struct LDStoreListener *listener;

    LD_ASSERT(store);
    LD_ASSERT(flagKey);
    LD_ASSERT(op);

    if (!(listener = LDAlloc(sizeof(*listener)))) {
        return false;
    }

    listener->key = LDStrDup(flagKey);

    if (!listener->key) {
        LDFree(listener);

        return false;
    }

    listener->fn = op;

    LDi_rwlock_wrlock(&store->lock);

    listener->next   = store->listeners;
    store->listeners = listener;

    LDi_rwlock_wrunlock(&store->lock);

    return true;
}

void
LDi_storeUnregisterListener(struct LDStore *const store,
    const char *const flagKey, LDlistenerfn op)
{
    struct LDStoreListener *listener, *previous;

    LD_ASSERT(store);
    LD_ASSERT(flagKey);
    LD_ASSERT(op);

    previous = NULL;

    LDi_rwlock_wrlock(&store->lock);

    for (listener = store->listeners; listener; listener = listener->next) {
        if (listener->fn == op && strcmp(flagKey, listener->key) == 0) {
            if (previous) {
                previous->next = listener->next;
            } else {
                store->listeners = listener->next;
            }

            LDFree(listener->key);
            LDFree(listener);
        } else {
            previous = listener;
        }
    }

    LDi_rwlock_wrunlock(&store->lock);
}
