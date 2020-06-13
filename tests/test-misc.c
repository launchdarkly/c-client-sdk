#include <launchdarkly/api.h>

#include "ldinternal.h"

static void
testGenerateUUIDv4()
{
    char buffer[LD_UUID_SIZE];
    /* This is mainly called as something for Valgrind to analyze */
    LD_ASSERT(LDi_UUIDv4(buffer));
}

static void
testStreamBackoff()
{
    double delay;

    delay = LDi_calculateStreamDelay(0);
    LD_ASSERT(delay == 0);

    delay = LDi_calculateStreamDelay(1);
    LD_ASSERT(delay == 1000);

    delay = LDi_calculateStreamDelay(2);
    LD_ASSERT(delay <= 4000 && delay >= 2000);

    delay = LDi_calculateStreamDelay(3);
    LD_ASSERT(delay <= 8000 && delay >= 4000);

    delay = LDi_calculateStreamDelay(10);
    LD_ASSERT(delay <= 30 * 1000 && delay >= 15 * 1000);

    delay = LDi_calculateStreamDelay(100000);
    LD_ASSERT(delay <= 30 * 1000 && delay >= 15 * 1000);
}

int
main()
{
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLogger);

    testGenerateUUIDv4();
    testStreamBackoff();

    return 0;
}
