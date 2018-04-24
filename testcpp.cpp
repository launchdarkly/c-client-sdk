
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

    LDUser *user = LDUserNew("user200");

    LDClient *client = LDClient::Init(config, user);

    while (!client->isInitialized()) {
        printf("not ready yet\n");
        sleep(1);
    }

    int delay = 0;
    while (true) {

        if (client->intVariation("bugcount", 10) < 5) {
            break;
        }
    
    if (client->boolVariation("sort.order", true)) {
        printf("sort order is true\n");
    } else {
        printf("sort order is false\n");
    }

    delay = 10;
    sleep(delay);
    
    printf("%d seconds up\n", delay);
    client->flush();
    }

    client->close();
    return 0;
}

