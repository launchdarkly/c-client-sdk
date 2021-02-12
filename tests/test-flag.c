#include <launchdarkly/api.h>

#include "ldinternal.h"

static void
testParseAndSerializeAllFields()
{
    struct LDFlag  flag;
    char *         flagString;
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

    LD_ASSERT(flagJSON1 = LDJSONDeserialize(flagString));

    LD_ASSERT(LDi_flag_parse(&flag, NULL, flagJSON1));
    LD_ASSERT(flagJSON2 = LDi_flag_to_json(&flag));

    LD_ASSERT(LDJSONCompare(flagJSON1, flagJSON2));

    LDJSONFree(flagJSON1);
    LDJSONFree(flagJSON2);
    LDi_flag_destroy(&flag);
}

int
main()
{
    LDBasicLoggerThreadSafeInitialize();
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLoggerThreadSafe);

    testParseAndSerializeAllFields();

    LDBasicLoggerThreadSafeShutdown();

    return 0;
};
