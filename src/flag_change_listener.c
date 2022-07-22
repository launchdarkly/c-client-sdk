#include "flag_change_listener.h"
#include "utlist.h"
#include <launchdarkly/memory.h>

#include <string.h>

enum ChangeListenerType {
    WithoutUserData,
    WithUserData
};

struct ChangeListener {
    /* Owned flag key; must be freed. */
    char *flag;
    /* Whether the callback has user data associated with it. */
    enum ChangeListenerType type;
    /* User-provided callback. The type member indicates which union member is set. */
    union {
        LDlistenerfn withoutUserData;
        LDlistenerUserDatafn withUserData;
    } callback;
    /* User data to be passed to the callback or NULL. */
    void * userData;
    /* Used by utlist.h macros. */
    struct ChangeListener *next;
};

static struct ChangeListener *
newListener(const char* flag, LDlistenerfn callback) {
    struct ChangeListener *listener = NULL;

    if (!(listener = LDAlloc(sizeof(struct ChangeListener)))) {
        return LDBooleanFalse;
    }

    listener->type = WithoutUserData;
    listener->callback.withoutUserData = callback;
    listener->flag = NULL;
    listener->userData = NULL;
    listener->next = NULL;

    if (!(listener->flag = LDStrDup(flag))) {
        LDFree(listener);
        return NULL;
    }

    return listener;
}

static struct ChangeListener *
newListenerUserData(const char* flag, LDlistenerUserDatafn callback, void *const userData) {
    struct ChangeListener *listener = NULL;

    if (!(listener = LDAlloc(sizeof(struct ChangeListener)))) {
        return LDBooleanFalse;
    }

    listener->type = WithUserData;
    listener->callback.withUserData = callback;
    listener->flag = NULL;
    listener->userData = userData;
    listener->next = NULL;

    if (!(listener->flag = LDStrDup(flag))) {
        LDFree(listener);
        return NULL;
    }

    return listener;
}

static void
freeListener(struct ChangeListener *listener) {
    LDFree(listener->flag);
    LDFree(listener);
}

/* Used by the utlist's LL_SEARCH macro to locate a ChangeListener. */
static int
flagcmp(struct ChangeListener *a, struct ChangeListener *b) {

    /* If the listeners have different flags, return that. */
    int cmp = strcmp(a->flag, b->flag);
    if (cmp != 0) {
        return cmp;
    }

    /* If one listener has user data and the other doesn't, they are unequal. */
    if (a->type != b->type) {
        return a->type - b->type > 0 ? 1 : -1;
    }

    /* If the listeners have the same flag & callback, they are equal. */
    if (a->type == WithUserData ? a->callback.withUserData == b->callback.withUserData : a->callback.withoutUserData == b->callback.withoutUserData) {
        return 0;
    }

    /* If the listeners have different user data, they're not equal. */
    if (a->type == WithUserData && a->userData != b->userData) {
        return a->userData > b->userData ? 1 : -1;
    }

    /* Otherwise, sort by the callback address to provide something stable.*/
    if (a->type == WithUserData ? ((unsigned long) a->callback.withUserData > (unsigned long) b->callback.withUserData) : ((unsigned long) a->callback.withoutUserData > (unsigned long) b->callback.withoutUserData)) {
        return 1;
    }
    return -1;
}

void
LDi_initListeners(struct ChangeListener** listeners) {
    /* Setting the list head to NULL is utlist's only requirement for operation. */
    *listeners = NULL;
}

void
LDi_freeListeners(struct ChangeListener** listeners) {
    struct ChangeListener *tmp, *listener;

    tmp = NULL;
    listener = NULL;

    if (*listeners) {
        LL_FOREACH_SAFE(*listeners, listener, tmp) {
            LL_DELETE(*listeners, listener);
            freeListener(listener);
        }
        *listeners = NULL;
    }
}

LDBoolean
LDi_listenerAdd(struct ChangeListener** listeners, const char* flag, LDlistenerfn callback) {
    struct ChangeListener *new, *existing;

    new = NULL;
    existing = NULL;

    if (!(new = newListener(flag, callback))) {
        return LDBooleanFalse;
    }

    LL_SEARCH(*listeners, existing, new, flagcmp);

    /* Ensure uniqueness of (flag, function pointer) combo. */
    if (existing) {
        freeListener(new);
        return LDBooleanTrue;
    }

    LL_PREPEND(*listeners, new);
    return LDBooleanTrue;
}

void
LDi_listenerRemove(struct ChangeListener** listeners, const char* flag, LDlistenerfn callback) {
    struct ChangeListener *tmp, *listener;

    tmp = NULL;
    listener = NULL;

    LL_FOREACH_SAFE(*listeners, listener, tmp) {
        if (strcmp(listener->flag, flag) == 0 && listener->type == WithoutUserData && listener->callback.withoutUserData == callback) {
            LL_DELETE(*listeners, listener);
            freeListener(listener);

            break; /* early out, since listenerInsert disallows duplicates */
        }
    }
}

LDBoolean
LDi_listenerUserDataAdd(struct ChangeListener** listeners, const char* flag, LDlistenerUserDatafn callback, void* const userData) {
    struct ChangeListener *new, *existing;

    new = NULL;
    existing = NULL;

    if (!(new = newListenerUserData(flag, callback, userData))) {
        return LDBooleanFalse;
    }

    LL_SEARCH(*listeners, existing, new, flagcmp);

    /* Ensure uniqueness of (flag, function pointer) combo. */
    if (existing) {
        freeListener(new);
        return LDBooleanTrue;
    }

    LL_PREPEND(*listeners, new);
    return LDBooleanTrue;
}

void
LDi_listenerUserDataRemove(struct ChangeListener** listeners, const char* flag, LDlistenerUserDatafn callback) {
    struct ChangeListener *tmp, *listener;

    tmp = NULL;
    listener = NULL;

    LL_FOREACH_SAFE(*listeners, listener, tmp) {
        if (strcmp(listener->flag, flag) == 0 && listener->type == WithUserData && listener->callback.withUserData == callback) {
            LL_DELETE(*listeners, listener);
            freeListener(listener);

            break; /* early out, since listenerInsert disallows duplicates */
        }
    }
}

void
LDi_listenersDispatch(struct ChangeListener* listeners, const char *flag, LDBoolean status) {
    struct ChangeListener *tmp, *listener;

    tmp = NULL;
    listener = NULL;

    LL_FOREACH_SAFE(listeners, listener, tmp) {
        if (strcmp(listener->flag, flag) == 0) {
            if (listener->type == WithUserData) {
                listener->callback.withUserData(flag, status, listener->userData);
            } else {
                listener->callback.withoutUserData(flag, status);
            }
        }
    }
}
