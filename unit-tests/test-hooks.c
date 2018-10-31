
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "ldapi.h"
#include "ldinternal.h"

void
logger(const char *s)
{
    printf("LD: %s", s);
}

bool fixed = false;
void
hook(const char *name, int change)
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

    LDConfig *config = LDConfigNew("authkey");
    config->offline = true;
    LDUser *user = LDUserNew("userX");
    LDClient *client = LDClientInit(config, user, 0);
    LDClientRegisterFeatureFlagListener(client, "bugcount", hook);

    char *testflags = "{ \"bugcount\": 1 }";
    LDClientRestoreFlags(client, testflags);

    fixed = false;

    char *patch = "{ \"key\": \"bugcount\", \"value\": 0 } }";
    LDi_onstreameventpatch(patch);

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
