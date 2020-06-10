#include <launchdarkly/api.h>

#include "ldinternal.h"

static void
testGenerateUUIDv4()
{
    char buffer[LD_UUID_SIZE];
    /* This is mainly called as something for Valgrind to analyze */
    LD_ASSERT(LDi_UUIDv4(buffer));
}

int
main()
{
    testGenerateUUIDv4();
}
