
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

void
statusupdate(int status)
{
    printf("The status is now %d\n", status);
}

int
main(int argc, char **argv)
{
    printf("Beginning tests\n");

    LDSetLogFunction(20, logger);

    LDSetClientStatusCallback(statusupdate);

    LDConfig *config = LDConfigNew("authkey");
    LDConfigSetOffline(config, true);

    LDUser *user = LDUserNew("user200");

    LDClient *client = LDClientInit(config, user);

    char *testflags = "{ \"sort.order\": { \"value\": false}, \"bugcount\": { \"value\": 0} , \"jj\": { \"value\": { \"ii\": 7 }} }";

    printf("Restoring flags\n");
    LDClientRestoreFlags(client, testflags);
    printf("Done restoring. Did the status change?\n");

    char *ss = LDClientSaveFlags(client);
    printf("INFO: the output json is %s\n", ss);
    LDFree(ss);

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

    LDNode *jnode = LDJSONVariation(client, "jj", NULL);
    LDNode *ii = LDNodeLookup(jnode, "ii");
    if (ii->type != LDNodeNumber || ii->n != 7) {
        printf("ERROR: the json was not as expected\n");
    }
    LDJSONRelease(jnode);
    jnode = LDJSONVariation(client, "missing", NULL);
    ii = LDNodeLookup(jnode, "ii");
    if (ii != NULL) {
        printf("ERROR: found some unexpected json\n");
    }
    LDJSONRelease(jnode);
    
    LDClientClose(client);

    printf("Completed all tests\n");

    extern unsigned long long LD_allocations, LD_frees;

    printf("Memory consumed: %lld allocs %lld frees\n", LD_allocations, LD_frees);

    return 0;
}

