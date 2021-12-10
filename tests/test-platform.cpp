#include "gtest/gtest.h"
#include "commonfixture.h"

extern "C" {
#include <launchdarkly/api.h>

#include "ldinternal.h"
}

// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class PlatformFixture : public CommonFixture {
};

TEST_F(PlatformFixture, DeviceId) {
    char *const deviceid = LDi_deviceid();

    /* The device ID is not available on all platforms.
     * For instance inside a docker container it may not be present.
     * Therefore there are no assertions in this test.
     */
    if (deviceid) {
        printf("Device ID is: %s", deviceid);
    } else {
        printf("ERROR: failed to get device ID");
    }

    LDFree(deviceid);
}

TEST_F(PlatformFixture, ConditionWaitTimeout) {
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
    ASSERT_GE(end - start, 1);
    ASSERT_LE(end - start, 3);

    LDi_cond_destroy(&condition);
    LDi_mutex_destroy(&lock);
}
