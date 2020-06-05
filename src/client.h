#pragma once

#include "uthash.h"

#include "config.h"
#include "user.h"
#include "store.h"

struct listener {
    LDlistenerfn fn;
    char *key;
    struct listener *next;
};

struct LDGlobal_i {
    struct LDClient *clientTable;
    struct LDClient *primaryClient;
    struct LDConfig *sharedConfig;
    struct LDUser *sharedUser;
    ld_rwlock_t sharedUserLock;
};

struct LDClient {
    struct LDGlobal_i *shared;
    char *mobileKey;
    ld_rwlock_t clientLock;
    bool offline;
    bool background;
    LDStatus status;
    struct listener *listeners;
    /* thread management */
    ld_thread_t eventThread;
    ld_thread_t pollingThread;
    ld_thread_t streamingThread;
    ld_cond_t eventCond;
    ld_cond_t pollCond;
    ld_cond_t streamCond;
    ld_mutex_t condMtx;
    /* streaming state */
    bool shouldstopstreaming;
    int streamhandle;
    struct EventProcessor *eventProcessor;
    struct LDStore store;
    /* init cond */
    ld_cond_t initCond;
    ld_mutex_t initCondMtx;
    UT_hash_handle hh;
};

struct LDClient *LDi_clientinitisolated(struct LDGlobal_i *shared,
    const char *mobileKey);
