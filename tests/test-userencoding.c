#include <stdio.h>

#include "ldapi.h"
#include "ldinternal.h"

void
testBasicSerialization()
{
    struct LDUser *user;
    struct LDJSON *custom, *tmp, *userJSON;
    char *userSerialized, *expected;

    LD_ASSERT(custom = LDNewObject());
    LD_ASSERT(tmp = LDNewNumber(10));
    LD_ASSERT(LDObjectSetKey(custom, "excellence", tmp));
    LD_ASSERT(tmp = LDNewBool(true));
    LD_ASSERT(LDObjectSetKey(custom, "bossmode", tmp));
    LD_ASSERT(tmp = LDNewText("krell"));
    LD_ASSERT(LDObjectSetKey(custom, "species", tmp));

    LD_ASSERT(user = LDUserNew("username"));
    LDUserSetFirstName(user, "Tsrif");
    LDUserSetLastName(user, "Tsal");
    LDUserSetAvatar(user, "pirate");
    LDUserSetCustomAttributesJSON(user, custom);

    LD_ASSERT(userJSON = LDi_userToJSON(NULL, user, true));
    LD_ASSERT(userSerialized = LDJSONSerialize(userJSON));

    expected =
        "{\"key\":\"username\",\"firstName\":\"Tsrif\",\"lastName\":\"Tsal\","
        "\"avatar\":\"pirate\",\"custom\":{\"excellence\":10,\"bossmode\":true,"
        "\"species\":\"krell\"}}";

    LD_ASSERT(strcmp(userSerialized, expected) == 0);

    LDJSONFree(userJSON);
    LDFree(userSerialized);
    LDUserFree(user);
}

void
testPrivateAttributes()
{
    struct LDUser *user;
    struct LDJSON *userJSON, *privateAttributes, *customAttributes, *tmp, *tmp2;
    char *userSerialized, *expected;

    LD_ASSERT(privateAttributes = LDNewArray());
    LD_ASSERT(tmp = LDNewText("count"));
    LD_ASSERT(LDArrayPush(privateAttributes, tmp));
    LD_ASSERT(tmp = LDNewText("avatar"));
    LD_ASSERT(LDArrayPush(privateAttributes, tmp));

    LD_ASSERT(customAttributes = LDNewObject());
    LD_ASSERT(tmp = LDNewNumber(23));
    LD_ASSERT(LDObjectSetKey(customAttributes, "count", tmp));
    LD_ASSERT(tmp = LDNewArray());
    LD_ASSERT(tmp2 = LDNewText("apple"));
    LD_ASSERT(LDArrayPush(tmp, tmp2));
    LD_ASSERT(LDObjectSetKey(customAttributes, "food", tmp));

    LD_ASSERT(user = LDUserNew("username"));
    LDUserSetFirstName(user, "Tsrif");
    LDUserSetAvatar(user, "pirate");
    LDUserSetCustomAttributesJSON(user, customAttributes);
    LDUserSetPrivateAttributes(user, privateAttributes);

    LD_ASSERT(userJSON = LDi_userToJSON(NULL, user, true));
    LD_ASSERT(userSerialized = LDJSONSerialize(userJSON));

    expected =
        "{\"key\":\"username\",\"firstName\":\"Tsrif\",\"custom\":"
        "{\"food\":[\"apple\"]},\"privateAttrs\":[\"avatar\",\"count\"]}";

    LD_ASSERT(strcmp(userSerialized, expected) == 0);

    LDJSONFree(userJSON);
    LDFree(userSerialized);
    LDUserFree(user);
}

int
main(int argc, char **argv)
{
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLogger);

    testBasicSerialization();
    testPrivateAttributes();

    return 0;
}
