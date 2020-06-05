#include <stdio.h>

#include "ldapi.h"
#include "ldinternal.h"

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
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLogger);

    struct LDConfig *const config = LDConfigNew("abc");
    LDConfigSetOffline(config, true);

    struct LDUser *const user = LDUserNew("userX");

    struct LDClient *const client = LDClientInit(config, user, 0);
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
    test1();

    return 0;
}
