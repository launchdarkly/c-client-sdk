#include "gtest/gtest.h"
#include "commonfixture.h"

extern "C" {
#include <launchdarkly/api.h>

#include "ldinternal.h"
}

// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class FlagFixture : public CommonFixture {
};

TEST_F(FlagFixture, ParseAndSerializeAllFields) {
    struct LDFlag flag;
    const char *flagString;
    struct LDJSON *flagJSON1, *flagJSON2;

    flagString =
            "{\n"
            "\"key\": \"a\",\n"
            "\"value\": 2,\n"
            "\"version\": 53,\n"
            "\"variation\": 3,\n"
            "\"flagVersion\": 45,\n"
            "\"trackEvents\": true,\n"
            "\"reason\": {\n"
            "\"kind\": \"ERROR\",\n"
            "\"errorKind\": \"WRONG_TYPE\"\n"
            "},\n"
            "\"debugEventsUntilDate\": 5000,\n"
            "\"deleted\": true\n"
            "}";

    ASSERT_TRUE(flagJSON1 = LDJSONDeserialize(flagString));

    ASSERT_TRUE(LDi_flag_parse(&flag, NULL, flagJSON1));
    ASSERT_TRUE(flagJSON2 = LDi_flag_to_json(&flag));

    ASSERT_TRUE(LDJSONCompare(flagJSON1, flagJSON2));

    LDJSONFree(flagJSON1);
    LDJSONFree(flagJSON2);
    LDi_flag_destroy(&flag);
}
