#include "gtest/gtest.h"
#include "commonfixture.h"

extern "C" {
#include "ldtime.h"
#include <launchdarkly/json.h>
#include <launchdarkly/memory.h>
}

// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class TimeFixture : public CommonFixture {
  protected:
    static LDTimestamp invalidTimestamp() {
        LDTimestamp a;
        a.ms = -1;
        return a;
    }
    static LDTimestamp uninitializedTimestamp() {
        LDTimestamp a;
        a.ms = -1231241412;
        return a;
    }
};

TEST_F(TimeFixture, InvalidTimestampIsInvalid) {
    LDTimestamp a = TimeFixture::invalidTimestamp();
    ASSERT_TRUE(LDTimestamp_IsInvalid(&a));
}

TEST_F(TimeFixture, ZeroTimestampIsNotInvalid) {
    LDTimestamp a = TimeFixture::uninitializedTimestamp();
    LDTimestamp_InitZero(&a);
    ASSERT_FALSE(LDTimestamp_IsInvalid(&a));
}

TEST_F(TimeFixture, UnixMillisTimestampIsNotInvalid) {
    LDTimestamp a = TimeFixture::uninitializedTimestamp();
    LDTimestamp_InitUnixMillis(&a, 0);
    ASSERT_FALSE(LDTimestamp_IsInvalid(&a));
}

TEST_F(TimeFixture, UnixSecondsTimestampIsNotInvalid) {
    LDTimestamp a = TimeFixture::uninitializedTimestamp();
    LDTimestamp_InitUnixSeconds(&a, 0);
    ASSERT_FALSE(LDTimestamp_IsInvalid(&a));
}


TEST_F(TimeFixture, CurrentTimestampIsNotInvalid) {
    LDTimestamp a = TimeFixture::uninitializedTimestamp();
    LDTimestamp_InitNow(&a);
    ASSERT_FALSE(LDTimestamp_IsInvalid(&a));
}

TEST_F(TimeFixture, TestComparisons) {
    LDTimestamp a, b, c, d;

    LDTimestamp_InitUnixMillis(&a, 0);
    LDTimestamp_InitUnixMillis(&b, 100);
    LDTimestamp_InitUnixMillis(&c, 200);
    LDTimestamp_InitUnixMillis(&d, 200);


    ASSERT_TRUE(LDTimestamp_Before(&a, &b));
    ASSERT_TRUE(LDTimestamp_Before(&a, &c));
    ASSERT_TRUE(LDTimestamp_Before(&b, &c));

    ASSERT_FALSE(LDTimestamp_Before(&c, &d));
    ASSERT_FALSE(LDTimestamp_Before(&d, &c));
    ASSERT_TRUE(LDTimestamp_Equal(&c, &d));
}

TEST_F(TimeFixture, TestTimestamp_AsUnixMillis) {
    time_t seconds = 10;
    double millis = (double) seconds * 1000;

    LDTimestamp a;
    LDTimestamp_InitUnixSeconds(&a, seconds);

    ASSERT_EQ(millis, LDTimestamp_AsUnixMillis(&a));
}

TEST_F(TimeFixture, TestTimestamp_MarshalJSON) {
    LDJSON *json;
    LDTimestamp a;

    LDTimestamp_InitUnixMillis(&a, 100);

    ASSERT_TRUE(json = LDTimestamp_MarshalUnixMillis(&a));
    char *jsonString = LDJSONSerialize(json);
    ASSERT_STREQ(jsonString, "100");

    LDJSONFree(json);
    LDFree(jsonString);
}


// Only checks that the Reset and Elapsed functions return true, as a rough sanity check that
// usage of the platform time APIs is supported.
TEST_F(TimeFixture, TestTimerSupport) {
    LDTimer a;
    double elapsed;
    ASSERT_TRUE(LDTimer_Reset(&a));
    ASSERT_TRUE(LDTimer_Elapsed(&a, &elapsed));
}

TEST_F(TimeFixture, TestTimerElapsed) {
    // Normally the monotonic timestamps won't be negative;
    // this is used to guarantee that elapsed will be >= 0 for the test.
    // Ideally this type of test would use a mocked time provider API.
    LDTimer a;
    a.ms = -10;
    double elapsed;
    ASSERT_TRUE(LDTimer_Elapsed(&a, &elapsed));
    ASSERT_GT(elapsed, 0);
}
