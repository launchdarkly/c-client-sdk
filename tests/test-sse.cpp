#include "commonfixture.h"
#include "gtest/gtest.h"

extern "C" {
#include <string.h>

#include "assertion.h"
#include "sse.h"
}

class SseFixture : public ::testing::Test {
};

static char nameBuffer[4096], bodyBuffer[4096];

static LDBoolean
mockDispatch(
    const char *const name, const char *const body, void *const context)
{
    memcpy(nameBuffer, name, strlen(name) + 1);
    memcpy(bodyBuffer, body, strlen(body) + 1);

    return LDBooleanTrue;
}

TEST_F(SseFixture, BasicEvent)
{
    struct LDSSEParser parser;

    LDSSEParserInitialize(&parser, mockDispatch, NULL);

    const char *const event =
        "event: delete\n"
        "data: hello\n\n";

    ASSERT_TRUE(LDSSEParserProcess(&parser, event, strlen(event)));
    ASSERT_STREQ(nameBuffer, "delete");
    ASSERT_STREQ(bodyBuffer, "hello");

    LDSSEParserDestroy(&parser);
}
