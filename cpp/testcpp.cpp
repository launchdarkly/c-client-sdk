
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
    LDSetLogFunction(5, logger);

    LDConfig *config = LDConfigNew("authkey");
    config->offline = true;

    LDUser *user = LDUserNew("user200");

    LDClient *client = LDClient::Init(config, user);
    client->restoreFlags("{ \"sort.order\": false, \"bugcount\": 0, \"jj\": { \"ii\": 7 } }");

    while (!client->isInitialized()) {
        printf("not ready yet\n");
        sleep(1);
    }

    int delay = 0;
    if (client->boolVariation("sort.order", true)) {
        printf("sort order is true\n");
    } else {
        printf("sort order is false\n");
    }

    LDNode *jnode = client->JSONVariation("jj", NULL);
    LDNode *ii = jnode->lookup("ii");
    if (ii->type != LDNodeNumber || ii->n != 7) {
        printf("ERROR: the json was not as expected\n");
    }
    jnode->release();

    client->flush();

    client->close();

    printf("CXX tests completed\n");
    return 0;
}

