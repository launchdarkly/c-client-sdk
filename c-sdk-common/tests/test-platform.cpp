#include "commonfixture.h"
#include "gtest/gtest.h"

extern "C" {
#include <launchdarkly/memory.h>

#include "assertion.h"
#include "concurrency.h"
#include "utility.h"
}

class PlatformFixture : public CommonFixture {
};

TEST_F(PlatformFixture, Monotonic)
{
    double past, present;

    ASSERT_TRUE(LDi_getMonotonicMilliseconds(&past));
    ASSERT_TRUE(LDi_getMonotonicMilliseconds(&present));

    ASSERT_GE(present, past);
}

TEST_F(PlatformFixture, GetUnixMilliseconds)
{
    double now;
    ASSERT_TRUE(LDi_getUnixMilliseconds(&now));
}

TEST_F(PlatformFixture, SleepMinimum)
{
    double past, present;

    ASSERT_TRUE(LDi_getMonotonicMilliseconds(&past));

    ASSERT_TRUE(LDi_sleepMilliseconds(50));

    ASSERT_TRUE(LDi_getMonotonicMilliseconds(&present));
    /* monotonic clock accurate to within 10 ms */
    ASSERT_GE(present - past, 40);
}

static THREAD_RETURN
threadDoNothing(void *const empty)
{
    LD_ASSERT(!empty);

    return THREAD_RETURN_DEFAULT;
}

TEST_F(PlatformFixture, ThreadStartJoin)
{
    ld_thread_t thread;

    ASSERT_TRUE(LDi_thread_create(
        &thread,
        threadDoNothing,
        NULL));
    ASSERT_TRUE(LDi_thread_join(&thread));
}

TEST_F(PlatformFixture, RWLock)
{
    ld_rwlock_t lock;

    ASSERT_TRUE(LDi_rwlock_init(&lock));

    ASSERT_TRUE(LDi_rwlock_rdlock(&lock));
    ASSERT_TRUE(LDi_rwlock_rdunlock(&lock));

    ASSERT_TRUE(LDi_rwlock_wrlock(&lock));
    ASSERT_TRUE(LDi_rwlock_wrunlock(&lock));

    ASSERT_TRUE(LDi_rwlock_destroy(&lock));
}

struct Context
{
    ld_rwlock_t lock;
    LDBoolean   flag;
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

TEST_F(PlatformFixture, Concurrency)
{
    ld_thread_t thread;

    struct Context context;
    context.flag = LDBooleanFalse;

    ASSERT_TRUE(LDi_rwlock_init(&context.lock));
    ASSERT_TRUE(LDi_thread_create(&thread, threadGoAwait, &context));

    ASSERT_TRUE(LDi_sleepMilliseconds(25));
    ASSERT_TRUE(LDi_rwlock_wrlock(&context.lock));
    context.flag = LDBooleanTrue;
    ASSERT_TRUE(LDi_rwlock_wrunlock(&context.lock));

    while (LDBooleanTrue) {
        LDBoolean status;

        ASSERT_TRUE(LDi_rwlock_wrlock(&context.lock));
        status = context.flag;
        ASSERT_TRUE(LDi_rwlock_wrunlock(&context.lock));

        if (!status) {
            break;
        }

        ASSERT_TRUE(LDi_sleepMilliseconds(1));
    }

    ASSERT_TRUE(LDi_thread_join(&thread));
    ASSERT_TRUE(LDi_rwlock_destroy(&context.lock));
}

/* possible failure but very unlikely */
TEST_F(PlatformFixture, RNG)
{
    unsigned int rng1, rng2;

    rng1 = 0;
    rng2 = 0;

    ASSERT_TRUE(LDi_random(&rng1));
    ASSERT_NE(rng1, 0);
    ASSERT_TRUE(LDi_random(&rng2));
    ASSERT_NE(rng2, 0);
    ASSERT_NE(rng1, rng2);
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

TEST_F(PlatformFixture, ConditionVars)
{
    ld_cond_t   condition;
    ld_thread_t thread;

    ASSERT_TRUE(LDi_mutex_init(&conditionTestLock));
    LDi_cond_init(&condition);

    ASSERT_TRUE(LDi_mutex_lock(&conditionTestLock));
    ASSERT_TRUE(LDi_thread_create(&thread, threadSignalCondition, &condition));

    ASSERT_TRUE(LDi_cond_wait(&condition, &conditionTestLock, 1000));
    ASSERT_TRUE(LDi_mutex_unlock(&conditionTestLock));

    LDi_cond_destroy(&condition);
    ASSERT_TRUE(LDi_mutex_destroy(&conditionTestLock));
    ASSERT_TRUE(LDi_thread_join(&thread));
}
