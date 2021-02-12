#pragma once

#include "uthash.h"

#include "config.h"
#include "store.h"
#include "user.h"

struct LDGlobal_i
{
    struct LDClient *clientTable;
    struct LDClient *primaryClient;
    struct LDConfig *sharedConfig;
    struct LDUser *  sharedUser;
    ld_rwlock_t      sharedUserLock;
};

struct LDClient
{
    struct LDGlobal_i *    shared;
    char *                 mobileKey;
    ld_rwlock_t            clientLock;
    bool                   offline;
    bool                   background;
    LDStatus               status;
    ld_thread_t            eventThread;
    ld_thread_t            pollingThread;
    ld_thread_t            streamingThread;
    ld_cond_t              eventCond;
    ld_cond_t              pollCond;
    ld_cond_t              streamCond;
    ld_mutex_t             condMtx;
    bool                   shouldstopstreaming;
    int                    streamhandle;
    struct EventProcessor *eventProcessor;
    struct LDStore         store;
    ld_cond_t              initCond;
    ld_mutex_t             initCondMtx;
    UT_hash_handle         hh;
};

struct LDClient *
LDi_clientInitIsolated(
    struct LDGlobal_i *const shared, const char *const mobileKey);

void
clientCloseIsolated(struct LDClient *const client);
