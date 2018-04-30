
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "ldapi.h"

void
logger(const char *s)
{
    printf("LD says %s\n", s);
}

int
main(int argc, char **argv)
{
    printf("back to basics\n");

    LD_SetLogFunction(20, logger);

    LDConfig *config = LDConfigNew("authkey");
    config->offline = true;

    LDUser *user = LDUserNew("user200");

    LDClient *client = LDClientInit(config, user);

    char *testflags = "{ \"sort.order\": false }";

    LDClientRestoreFlags(client, testflags);

    while (!LDClientIsInitialized(client)) {
        printf("not ready yet\n");
        sleep(1);
    }

    int delay = 0;
    while (true) {
    if (LDIntVariation(client, "bugcount", 10) > 5) {
        printf("it's greater than five\n");
    } else {
        break;
    }
    if (LDBoolVariation(client, "sort.order", true)) {
        printf("sort order is true\n");
    } else {
        printf("sort order is false\n");
    }

    delay = 10;
    sleep(delay);
    
    printf("%d seconds up\n", delay);
    LDClientFlush(client);
    }

    LDClientClose(client);

    return 0;
}

