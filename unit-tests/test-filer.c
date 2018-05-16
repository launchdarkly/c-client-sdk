
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

void *
fake_opener(void *ctx, const char *name, const char *mode, size_t len)
{
    return "the handle";
}

void
fake_closer(void *handle)
{
    if (strcmp(handle, "the handle") != 0) {
        printf("ERROR: something bad happened to the handle\n");
    }
}

bool gotcallback = false;

bool
fake_stringwriter(void *handle, const char *data)
{
    gotcallback = true;
}

void
test1(void)
{
    LD_filer_setfns(NULL, LD_filer_open, NULL /* no writer */, LD_filer_readstring, LD_filer_close);

    LDConfig *config = LDConfigNew("authkey");
    config->offline = true;
    LDUser *user = LDUserNew("fileuser");
    LDClient *client = LDClientInit(config, user);
    
    char buffer[256];
    LDStringVariation(client, "filedata", "incorrect", buffer, sizeof(buffer));
    if (strcmp(buffer, "as expected") != 0) {
        printf("ERROR: didn't load file data\n");
    }
    LDClientClose(client);

}

void
test2(void)
{
    LD_filer_setfns(NULL, fake_opener, fake_stringwriter, NULL, fake_closer);

    LDConfig *config = LDConfigNew("authkey");
    config->offline = true;
    LDUser *user = LDUserNew("fakeuser");
    LDClient *client = LDClientInit(config, user);
    char *putflags = "{ \"bgcolor\": { \"value\": \"red\", \"version\": 1 } }";
    LDi_onstreameventput(putflags);

    if (!gotcallback) {
        printf("ERROR: flag update didn't call writer\n");
    }

    LDClientClose(client);

}

int
main(int argc, char **argv)
{
    printf("Beginning tests\n");

    LDSetLogFunction(20, logger);

    test1();

    test2();

    printf("Completed all tests\n");
    return 0;
}

