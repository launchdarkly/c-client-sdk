#include "flag_change_listener.h"
#include "utlist.h"
#include <launchdarkly/memory.h>

#include <string.h>

struct ChangeListener {
    /* Owned flag key; must be freed. */
    char *flag;
    /* User-provided callback. */
    LDlistenerfn callback;
    /* Used by utlist.h macros. */
    struct ChangeListener *next;
};

static struct ChangeListener *
newListener(const char* flag, LDlistenerfn callback) {
    struct ChangeListener *listener = NULL;

    if (!(listener = LDAlloc(sizeof(struct ChangeListener)))) {
        return LDBooleanFalse;
    }

    listener->callback = callback;
    listener->flag = NULL;
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

    /* If the listeners have the same flag & callback, they are equal. */
    if (a->callback == b->callback) {
        return 0;
    }

    /* Otherwise, sort by the callback address to provide something stable.*/
    if ((unsigned long) a->callback > (unsigned long) b->callback) {
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
        if (strcmp(listener->flag, flag) == 0 && listener->callback == callback) {
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
            listener->callback(flag, status);
        }
    }
}
