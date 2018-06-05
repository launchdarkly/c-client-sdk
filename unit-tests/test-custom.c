
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
    user->custom = LDNodeCreateHash();
    LDNodeAddNumber(&user->custom, "excellence", 10);
    LDNodeAddBool(&user->custom, "bossmode", true);
    LDNodeAddString(&user->custom, "species", "krell");

    cJSON *json = LDi_usertojson(user);
    char *str = cJSON_PrintUnformatted(json);

    char *expected = "{\"key\":\"username\",\"firstName\":\"Tsrif\",\"lastName\":\"Tsal\","
    "\"avatar\":\"pirate\",\"custom\":{\"excellence\":10,\"bossmode\":true,\"species\":\"krell\"}}";

    if (strcmp(str, expected) != 0) {
        printf("ERROR: User json %s was not expected\n", str);
    }

    LDi_freeuser(user);

}

/*
 * test setting json directly. also has a list of custom attributes.
 */
void
test2(void)
{
    
    LDUser *user = LDUserNew("username");
    LDUserSetFirstName(user, "Tsrif");
    LDUserSetLastName(user, "Tsal");
    LDUserSetAvatar(user, "pirate");
    LDUserSetCustomAttributesJSON(user, "{\"toppings\": [\"pineapple\", \"ham\"]}");
    
    cJSON *json = LDi_usertojson(user);
    char *str = cJSON_PrintUnformatted(json);

    char *expected = "{\"key\":\"username\",\"firstName\":\"Tsrif\",\"lastName\":\"Tsal\","
    "\"avatar\":\"pirate\",\"custom\":{\"toppings\":[\"pineapple\",\"ham\"]}}";

    if (strcmp(str, expected) != 0) {
        printf("ERROR: User json %s was not expected\n", str);
    }

    LDi_freeuser(user);
}

int
main(int argc, char **argv)
{
    printf("Beginning tests\n");

    LDSetLogFunction(1, logger);

    test1();

    test2();

    extern unsigned long long LD_allocations, LD_frees;

    // printf("Memory consumed: %lld allocs %lld frees\n", LD_allocations, LD_frees);

    printf("Completed all tests\n");
    return 0;
}

