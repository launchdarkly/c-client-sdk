
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "ldapi.h"
#include "ldinternal.h"


int
main(int argc, char **argv)
{
    printf("Beginning tests\n");

    LDConfig *config = LDConfigNew("authkey");
    config->offline = true;
    LDConfigAddPrivateAttribute(config, "password");
    LDUser *user = LDUserNew("user200");
    LDClient *client = LDClientInit(config, user);
    char *testflags = "{ \"bgcolor\": \"red\", \"password\": \"secret\" }";
    LDClientRestoreFlags(client, testflags);

    char buffer[256];
    LDStringVariation(client, "bgcolor", "green", buffer, sizeof(buffer));
    LDStringVariation(client, "password", "uhoh", buffer, sizeof(buffer));

    char *data = LDi_geteventdata();
    if (strstr(data, "bgcolor") == NULL) {
        printf("ERROR: Where did bgcolor go?\n");
    }
    if (strstr(data, "password") != NULL) {
        printf("ERROR: How did password get in the data?\n");
    }

    LDClientClose(client);
    printf("Completed all tests\n");
    return 0;
}

