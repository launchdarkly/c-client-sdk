#include <stdio.h>

#include "ldapi.h"
#include "ldinternal.h"

bool gotcallback = false;

bool
fake_stringwriter(void *context, const char *name, const char *data)
{
    gotcallback = true;
    return true;
}

/*
 * Read flags from an external file and check for expected value.
 */
void
test1(void)
{
    LD_store_setfns(NULL, NULL /* no writer */, LD_store_fileread);

    struct LDConfig *const config = LDConfigNew("abc");
    LDConfigSetOffline(config, true);

    struct LDUser *const user = LDUserNew("fileuser");

    struct LDClient *const client = LDClientInit(config, user, 0);

    char buffer[256];
    LDStringVariation(client, "filedata", "incorrect", buffer, sizeof(buffer));
    if (strcmp(buffer, "as expected") != 0) {
        printf("ERROR: didn't load file data\n");
    }

    char *patch = "{ \"key\": \"filedata\", \"value\": \"updated\", \"version\": 2 } }";
    LDi_onstreameventpatch(client, patch);
    LDStringVariation(client, "filedata", "incorrect", buffer, sizeof(buffer));
    if (strcmp(buffer, "as expected") != 0) {
        printf("ERROR: applied stale patch\n");
    }

    patch = "{ \"key\": \"filedata\", \"value\": \"updated\", \"version\": 4 } }";
    LDi_onstreameventpatch(client, patch);
    LDStringVariation(client, "filedata", "incorrect", buffer, sizeof(buffer));
    if (strcmp(buffer, "updated") != 0) {
        printf("ERROR: didn't apply good patch\n");
    }

    LDClientClose(client);
}

/*
 * Test that flags get written out after receiving an update.
 */
void
test2(void)
{
    LD_store_setfns(NULL, fake_stringwriter, NULL);

    struct LDConfig *const config = LDConfigNew("abc");
    LDConfigSetOffline(config, true);

    struct LDUser *const user = LDUserNew("fakeuser");

    struct LDClient *const client = LDClientInit(config, user, 0);

    const char *const putflags = "{ \"bgcolor\": { \"value\": \"red\", \"version\": 1 } }";

    LDi_onstreameventput(client, putflags);

    if (!gotcallback) {
        printf("ERROR: flag update didn't call writer\n");
    }

    LDClientClose(client);
}

int
main(int argc, char **argv)
{
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLogger);

    test1();

    test2();

    return 0;
}
