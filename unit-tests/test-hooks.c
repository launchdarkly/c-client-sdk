
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "ldapi.h"
#include "ldinternal.h"

void
logger(const char *const s)
{
    printf("LD: %s", s);
}

bool fixed = false;

void
hook(const char *const name, const int change)
{
    fixed = true;
}

/*
 * Verify that the hook is run when a patch is received.
 * fixed should be set to true.
 */
void
test1(void)
{
    LDSetLogFunction(1, logger);

    LDConfig *const config = LDConfigNew("authkey");
    LDConfigSetOffline(config, true);

    LDUser *const user = LDUserNew("userX");

    LDClient *const client = LDClientInit(config, user);
    LDClientRegisterFeatureFlagListener(client, "bugcount", hook);

    const char *const testflags = "{ \"bugcount\": 1 }";

    LDClientRestoreFlags(client, testflags);

    fixed = false;

    const char *const patch = "{ \"key\": \"bugcount\", \"value\": 0 } }";

    LDi_onstreameventpatch(client, patch);

    if (!fixed) {
        printf("ERROR: bugcount wasn't fixed\n");
    }

    LDClientClose(client);
}

int
main(int argc, char **argv)
{
    printf("Beginning tests\n");

    test1();

    printf("Completed all tests\n");
    return 0;
}
