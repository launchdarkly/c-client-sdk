
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "ldapi.h"

void
logger(const char *s)
{
    printf("LD: %s", s);
}

int
main(int argc, char **argv)
{
    printf("Beginning tests\n");

    LD_SetLogFunction(20, logger);

    LDConfig *config = LDConfigNew("authkey");
    config->offline = true;

    LDUser *user = LDUserNew("user200");

    LDClient *client = LDClientInit(config, user);

    char *testflags = "{ \"sort.order\": false, \"bugcount\": 0, \"jj\": { \"ii\": 7 } }";

    LDClientRestoreFlags(client, testflags);

    char *ss = LDClientSaveFlags(client);
    printf("INFO: the output json is %s\n", ss);

    while (!LDClientIsInitialized(client)) {
        printf("not ready yet\n");
        sleep(1);
    }

    int high, low;

    low = LDIntVariation(client, "bugcount", 1);
    high = LDIntVariation(client, "bugcount", 10);
    if (high != low) {
        printf("ERROR: bugcount inconsistent\n");
    }
    low = LDIntVariation(client, "missing", 1);
    high = LDIntVariation(client, "missing", 10);
    if (high == low) {
        printf("ERROR: default malfunction\n");
    }

    char buffer[10];
    /* deliberately check for truncation (no overflow) */
    LDStringVariation(client, "missing", "more than ten letters", buffer, sizeof(buffer));
    if (strcmp(buffer, "more than") != 0) {
        printf("ERROR: the string variation failed\n");
    }
    char *letters = LDStringVariationAlloc(client, "missing", "more than ten letters");
    if (strcmp(letters, "more than ten letters") != 0) {
        printf("ERROR: the string variation failed\n");
    }
    LDFree(letters);

    LDMapNode *jnode = LDJSONVariation(client, "jj");
    LDMapNode *ii = LDMapLookup(jnode, "ii");
    if (ii->type != LDNodeNumber || ii->n != 7) {
        printf("ERROR: the json was not as expected\n");
    }
    LDJSONRelease(jnode);
    
    LDClientClose(client);

    printf("Completed all tests\n");

    return 0;
}

