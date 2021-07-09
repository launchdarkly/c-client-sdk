#include <string.h>

#include "assertion.h"
#include "sse.h"

static char nameBuffer[4096], bodyBuffer[4096];

static LDBoolean
mockDispatch(const char *const name, const char *const body,
    void *const context)
{
    memcpy(nameBuffer, name, strlen(name) + 1);
    memcpy(bodyBuffer, body, strlen(body) + 1);

    return LDBooleanTrue;
}

static void
testBasicEvent(void)
{
    struct LDSSEParser parser;

    LDSSEParserInitialize(&parser, mockDispatch, NULL);

    const char *const event =
        "event: delete\n"
        "data: hello\n\n";

    LD_ASSERT(LDSSEParserProcess(&parser, event, strlen(event)));
    LD_ASSERT(strcmp(nameBuffer, "delete") == 0);
    LD_ASSERT(strcmp(bodyBuffer, "hello") == 0);

    LDSSEParserDestroy(&parser);
}

int
main(void)
{
    testBasicEvent();

    return 0;
}
