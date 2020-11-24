#include <stdio.h>

#include <launchdarkly/api.h>

#include "ldinternal.h"

static void
testDeviceId()
{
    char *const deviceid = LDi_deviceid();

    if (deviceid) {
        printf("Device ID is: %s", deviceid);
    } else {
        printf("ERROR: failed to get device ID");
    }

    LDFree(deviceid);
}

static void
testConditionWaitTimeout()
{
    int start, end;
    ld_cond_t condition;
    ld_mutex_t lock;

    LDi_cond_init(&condition);
    LDi_mutex_init(&lock);

    start = time(NULL);
    LDi_mutex_lock(&lock);
    LDi_cond_wait(&condition, &lock, 1000 * 2);
    LDi_mutex_unlock(&lock);
    end = time(NULL);
    LD_ASSERT(end - start > 1 && end - start < 3);

    LDi_cond_destroy(&condition);
    LDi_mutex_destroy(&lock);
}

int
main(int argc, char **argv)
{
    LDBasicLoggerThreadSafeInitialize();
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLoggerThreadSafe);

    testDeviceId();
    testConditionWaitTimeout();

    LDBasicLoggerThreadSafeShutdown();
}
