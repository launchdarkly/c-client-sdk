#include <stdio.h>

#include <launchdarkly/api.h>

#include "ldinternal.h"

bool callCount;

void
hook(const char *const name, const int change)
{
    callCount++;
}

void
testBasicHookUpsert(void)
{
    struct LDFlag  flag;
    struct LDStore store;

    callCount = 0;

    LD_ASSERT(LDi_storeInitialize(&store));
    LD_ASSERT(LDi_storeRegisterListener(&store, "test", hook));

    flag.key                  = LDStrDup("test");
    flag.value                = LDNewBool(true);
    flag.version              = 2;
    flag.variation            = 3;
    flag.trackEvents          = false;
    flag.reason               = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted              = false;

    LD_ASSERT(flag.key);
    LD_ASSERT(flag.value);

    LD_ASSERT(LDi_storeUpsert(&store, flag));

    LD_ASSERT(callCount == 1);

    LDi_storeDestroy(&store);
}

void
testBasicHookPut(void)
{
    struct LDFlag *flag;
    struct LDStore store;

    callCount = 0;

    LD_ASSERT(flag = LDAlloc(sizeof(struct LDFlag)));

    LD_ASSERT(LDi_storeInitialize(&store));
    LD_ASSERT(LDi_storeRegisterListener(&store, "test", hook));

    flag->key                  = LDStrDup("test");
    flag->value                = LDNewBool(true);
    flag->version              = 2;
    flag->variation            = 3;
    flag->trackEvents          = false;
    flag->reason               = NULL;
    flag->debugEventsUntilDate = 0;
    flag->deleted              = false;

    LD_ASSERT(flag->key);
    LD_ASSERT(flag->value);

    LD_ASSERT(LDi_storePut(&store, flag, 1));

    LD_ASSERT(callCount == 1);

    LDi_storeDestroy(&store);
}

int
main(int argc, char **argv)
{
    LDBasicLoggerThreadSafeInitialize();
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLoggerThreadSafe);

    testBasicHookUpsert();
    testBasicHookPut();

    LDBasicLoggerThreadSafeShutdown();

    return 0;
}
