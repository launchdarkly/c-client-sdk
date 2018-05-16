
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


int
main(int argc, char **argv)
{
    printf("Beginning tests\n");

    LDSetLogFunction(20, logger);

    LD_filer_setfns(NULL, LD_filer_open, NULL /* no writer */, LD_filer_readstring, LD_filer_close);

    LDConfig *config = LDConfigNew("authkey");
    config->offline = true;
    LDConfigAddPrivateAttribute(config, "password");
    LDUser *user = LDUserNew("fileuser");
    LDClient *client = LDClientInit(config, user);
    
    char buffer[256];
    LDStringVariation(client, "filedata", "incorrect", buffer, sizeof(buffer));
    if (strcmp(buffer, "as expected") != 0) {
        printf("ERROR: didn't load file data\n");
    }

    LDClientClose(client);
    printf("Completed all tests\n");
    return 0;
}

