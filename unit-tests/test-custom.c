
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

/*
 * Test that turning a user into json looks like we expect.
 */
void
test1(void)
{
    
    LDUser *user = LDUserNew("username");
    LDSetString(&user->firstName, "Tsrif");
    LDSetString(&user->lastName, "Tsal");
    LDSetString(&user->avatar, "pirate");
    user->custom = LDMapCreate();
    LDMapAddNumber(&user->custom, "excellence", 10);
    LDMapAddBool(&user->custom, "bossmode", true);
    LDMapAddString(&user->custom, "species", "krell");

    cJSON *json = LDi_usertojson(user);
    char *str = cJSON_PrintUnformatted(json);

    char *expected = "{\"key\":\"username\",\"firstName\":\"Tsrif\",\"lastName\":\"Tsal\","
    "\"avatar\":\"pirate\",\"custom\":{\"excellence\":10,\"bossmode\":true,\"species\":\"krell\"}}";

    if (strcmp(str, expected) != 0) {
        printf("ERROR: User json %s was not expected\n", str);
    }

}

int
main(int argc, char **argv)
{
    printf("Beginning tests\n");

    LDSetLogFunction(1, logger);

    test1();

    printf("Completed all tests\n");
    return 0;
}

