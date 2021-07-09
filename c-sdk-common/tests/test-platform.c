#include <launchdarkly/memory.h>

#include "assertion.h"
#include "concurrency.h"
#include "utility.h"

static void
testMonotonic()
{
    double past, present;

    LD_ASSERT(LDi_getMonotonicMilliseconds(&past));
    LD_ASSERT(LDi_getMonotonicMilliseconds(&present));

    LD_ASSERT(present >= past);
}

static void
testGetUnixMilliseconds()
{
    double now;
    LD_ASSERT(LDi_getUnixMilliseconds(&now));
}

static void
testSleepMinimum()
{
    double past, present;

    LD_ASSERT(LDi_getMonotonicMilliseconds(&past));

    LD_ASSERT(LDi_sleepMilliseconds(50));

    LD_ASSERT(LDi_getMonotonicMilliseconds(&present));
    /* monotonic clock accurate to within 10 ms */
    LD_ASSERT(present - past >= 40);
}

static THREAD_RETURN
threadDoNothing(void *const empty)
{
    LD_ASSERT(!empty);

    return THREAD_RETURN_DEFAULT;
}

static void
testThreadStartJoin()
{
    ld_thread_t thread;

    LD_ASSERT(LDi_thread_create(&thread, threadDoNothing, NULL));
    LD_ASSERT(LDi_thread_join(&thread));
}

static void
testRWLock()
{
    ld_rwlock_t lock;

    LD_ASSERT(LDi_rwlock_init(&lock));

    LD_ASSERT(LDi_rwlock_rdlock(&lock));
    LD_ASSERT(LDi_rwlock_rdunlock(&lock));

    LD_ASSERT(LDi_rwlock_wrlock(&lock));
    LD_ASSERT(LDi_rwlock_wrunlock(&lock))

    LD_ASSERT(LDi_rwlock_destroy(&lock));
}

struct Context {
    ld_rwlock_t lock;
    LDBoolean flag;
};

static THREAD_RETURN
threadGoAwait(void *const rawcontext)
{
    struct Context *context = (struct Context *)rawcontext;

    while (LDBooleanTrue) {
        LD_ASSERT(LDi_rwlock_wrlock(&context->lock));
        if (context->flag) {
            context->flag = LDBooleanFalse;
            LD_ASSERT(LDi_rwlock_wrunlock(&context->lock));
            break;
        }
        LD_ASSERT(LDi_rwlock_wrunlock(&context->lock));

        LD_ASSERT(LDi_sleepMilliseconds(1));
    }

    return THREAD_RETURN_DEFAULT;
}

static void
testConcurrency()
{
    ld_thread_t thread;

    struct Context context;
    context.flag = LDBooleanFalse;

    LD_ASSERT(LDi_rwlock_init(&context.lock));
    LD_ASSERT(LDi_thread_create(&thread, threadGoAwait, &context));

    LD_ASSERT(LDi_sleepMilliseconds(25));
    LD_ASSERT(LDi_rwlock_wrlock(&context.lock));
    context.flag = LDBooleanTrue;
    LD_ASSERT(LDi_rwlock_wrunlock(&context.lock));

    while (LDBooleanTrue) {
        LDBoolean status;

        LD_ASSERT(LDi_rwlock_wrlock(&context.lock));
        status = context.flag;
        LD_ASSERT(LDi_rwlock_wrunlock(&context.lock));

        if (!status) {
            break;
        }

        LD_ASSERT(LDi_sleepMilliseconds(1));
    }

    LD_ASSERT(LDi_thread_join(&thread));
    LD_ASSERT(LDi_rwlock_destroy(&context.lock));
}

/* possible failure but very unlikely */
static void
testRNG()
{
    unsigned int rng1, rng2;

    rng1 = 0;
    rng2 = 0;

    LD_ASSERT(LDi_random(&rng1));
    LD_ASSERT(rng1 != 0);
    LD_ASSERT(LDi_random(&rng2));
    LD_ASSERT(rng2 != 0);
    LD_ASSERT(rng1 != rng2);
}

static ld_mutex_t conditionTestLock;

static THREAD_RETURN
threadSignalCondition(void *const condition)
{
    LD_ASSERT(condition);

    LD_ASSERT(LDi_mutex_lock(&conditionTestLock));
    LDi_cond_signal((ld_cond_t *)condition);
    LD_ASSERT(LDi_mutex_unlock(&conditionTestLock));

    return THREAD_RETURN_DEFAULT;
}

static void
testConditionVars()
{
    ld_cond_t condition;
    ld_thread_t thread;

    LD_ASSERT(LDi_mutex_init(&conditionTestLock));
    LDi_cond_init(&condition);

    LD_ASSERT(LDi_mutex_lock(&conditionTestLock));
    LD_ASSERT(LDi_thread_create(&thread, threadSignalCondition, &condition));

    LD_ASSERT(LDi_cond_wait(&condition, &conditionTestLock, 1000));
    LD_ASSERT(LDi_mutex_unlock(&conditionTestLock));

    LDi_cond_destroy(&condition);
    LD_ASSERT(LDi_mutex_destroy(&conditionTestLock));
    LD_ASSERT(LDi_thread_join(&thread));
}

int
main()
{
    LDBasicLoggerThreadSafeInitialize();
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLoggerThreadSafe);
    LDGlobalInit();

    testMonotonic();
    testGetUnixMilliseconds();
    testSleepMinimum();
    testThreadStartJoin();
    testRWLock();
    testConcurrency();
    testRNG();
    testConditionVars();

    LDBasicLoggerThreadSafeShutdown();

    return 0;
}
