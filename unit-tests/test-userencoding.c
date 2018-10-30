
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

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
test1(LDClient *const client)
{
    LDUser *const user = LDUserNew("username");
    LDUserSetFirstName(user, "Tsrif");
    LDUserSetLastName(user, "Tsal");
    LDUserSetAvatar(user, "pirate");
    user->custom = LDNodeCreateHash();
    LDNodeAddNumber(&user->custom, "excellence", 10);
    LDNodeAddBool(&user->custom, "bossmode", true);
    LDNodeAddString(&user->custom, "species", "krell");

    cJSON *const json = LDi_usertojson(client, user, true);
    const char *const str = cJSON_PrintUnformatted(json);

    const char *const expected = "{\"key\":\"username\",\"firstName\":\"Tsrif\",\"lastName\":\"Tsal\","
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
test2(LDClient *const client)
{
    LDUser *const user = LDUserNew("username");
    LDUserSetFirstName(user, "Tsrif");
    LDUserSetLastName(user, "Tsal");
    LDUserSetAvatar(user, "pirate");
    LDUserSetCustomAttributesJSON(user, "{\"toppings\": [\"pineapple\", \"ham\"]}");

    cJSON *const json = LDi_usertojson(client, user, true);
    const char *const str = cJSON_PrintUnformatted(json);

    const char *const expected = "{\"key\":\"username\",\"firstName\":\"Tsrif\",\"lastName\":\"Tsal\","
        "\"avatar\":\"pirate\",\"custom\":{\"toppings\":[\"pineapple\",\"ham\"]}}";

    if (strcmp(str, expected) != 0) {
        printf("ERROR: User json %s was not expected\n", str);
    }

    LDi_freeuser(user);
}

/*
 * Test private attributes
 */
void
test3(LDClient *const client)
{
    LDUser *const user = LDUserNew("username");
    LDUserSetFirstName(user, "Tsrif");
    LDUserSetAvatar(user, "pirate");
    LDUserSetCustomAttributesJSON(user, "{\"food\": [\"apple\"], \"count\": 23}");
    LDUserAddPrivateAttribute(user, "count");
    LDUserAddPrivateAttribute(user, "avatar");

    cJSON *const json = LDi_usertojson(client, user, true);
    const char *const str = cJSON_PrintUnformatted(json);

    const char *const expected = "{\"key\":\"username\",\"firstName\":\"Tsrif\",\"custom\":{\"food\":[\"apple\"]},\"privateAttrs\":[\"avatar\",\"count\"]}";

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

    LDUser *const user = LDUserNew("");

    LDConfig *const config = LDConfigNew("authkey");
    config->offline = true;

    LDClient *const client = LDClientInit(config, user, 0);

    test1(client);

    test2(client);

    test3(client);

    extern unsigned long long LD_allocations, LD_frees;

    // printf("Memory consumed: %lld allocs %lld frees\n", LD_allocations, LD_frees);

    printf("Completed all tests\n");
    return 0;
}
