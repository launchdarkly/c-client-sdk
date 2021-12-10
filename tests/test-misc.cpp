#include "gtest/gtest.h"
#include "commonfixture.h"

extern "C" {
#include <launchdarkly/api.h>

#include "ldinternal.h"
}

// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class MiscFixture : public CommonFixture {
};

TEST_F(MiscFixture, GenerateUUIDv4) {
    char buffer[LD_UUID_SIZE];
    /* This is mainly called as something for Valgrind to analyze */
    ASSERT_TRUE(LDi_UUIDv4(buffer));
}

TEST_F(MiscFixture, StreamBackoff) {
    double delay;

    delay = LDi_calculateStreamDelay(0);
    ASSERT_EQ(delay, 0);

    delay = LDi_calculateStreamDelay(1);
    ASSERT_EQ(delay, 1000);

    delay = LDi_calculateStreamDelay(2);
    ASSERT_LE(delay, 4000);
    ASSERT_GE(delay, 2000);

    delay = LDi_calculateStreamDelay(3);
    ASSERT_LE(delay, 8000);
    ASSERT_GE(delay, 4000);

    delay = LDi_calculateStreamDelay(10);
    ASSERT_LE(delay, 30 * 1000);
    ASSERT_GE(delay, 15 * 1000);

    delay = LDi_calculateStreamDelay(100000);
    ASSERT_LE(delay, 30 * 1000);
    ASSERT_GE(delay, 15 * 1000);
}
