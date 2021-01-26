#include <stdio.h>

#include <launchdarkly/api.h>

#include "assertion.h"
#include "concurrency.h"
#include "utility.h"

#define THREAD_COUNT 8
#define TOTAL_OPS 25000000 /* 25 million */
const unsigned int PER_THREAD_OPS = TOTAL_OPS / THREAD_COUNT;

ld_mutex_t lock;
struct LDClient *client;

static THREAD_RETURN
doEvals_thread(void *const unused)
{
    unsigned int i;

    LD_ASSERT(unused == NULL);

    /* blocks until all threads are created */
    LDi_mutex_lock(&lock);
    LDi_mutex_unlock(&lock);

    for (i = 0; i < PER_THREAD_OPS; i++) {
        LD_ASSERT(LDBoolVariation(client, "test", false) == false);
    }

    return THREAD_RETURN_DEFAULT;
}

int
main()
{
    struct LDUser *user;
    struct LDConfig *config;
    ld_thread_t threads[THREAD_COUNT];
    size_t i;
    double start, finish, nanoseconds;

    LD_ASSERT(config = LDConfigNew("key"));
    LDConfigSetOffline(config, true);

    LD_ASSERT(user = LDUserNew("user"));

    LD_ASSERT(client = LDClientInit(config, user, 0));
    
    LDi_mutex_init(&lock);
    /* blocks until all threads are created */
    LDi_mutex_lock(&lock);

    for (i = 0; i < sizeof(threads) / sizeof(threads[0]); i++) {
        LDi_thread_create(&threads[i], doEvals_thread, NULL);
    }

    LD_ASSERT(LDi_getMonotonicMilliseconds(&start));

    LDi_mutex_unlock(&lock);

    for (i = 0; i < sizeof(threads) / sizeof(threads[0]); i++) {
        LDi_thread_join(&threads[i]);
    }

    LD_ASSERT(LDi_getMonotonicMilliseconds(&finish));

    nanoseconds = ((finish - start) * 1000000) / TOTAL_OPS;
    start /= 1000;
    finish /= 1000;

    printf("duration seconds %f ns/op %f\n", finish - start, nanoseconds);

    LDi_mutex_destroy(&lock);

    LDClientClose(client);

    return 0;
}
