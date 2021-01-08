#include <stdio.h>

#include <launchdarkly/api.h>

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

void
testGeneratesKeyForNullKey()
{
    struct LDUser *user;

    LD_ASSERT(user = LDUserNew(NULL));
    LD_ASSERT(user->key != NULL);

    LDUserFree(user);
}

void
testSettersAndDefaults()
{
    struct LDUser *user;
    struct LDJSON *attributes, *tmp;

    LD_ASSERT(user = LDUserNew("a"));

    LD_ASSERT(user->anonymous == false);
    LDUserSetAnonymous(user, true);
    LD_ASSERT(user->anonymous == true);

    #define testStringField(method, field)                                     \
        LD_ASSERT(field == NULL);                                              \
        LD_ASSERT(method(user, "alice"));                                      \
        LD_ASSERT(strcmp(field, "alice") == 0);                                \
        LD_ASSERT(method(user, "bob"));                                        \
        LD_ASSERT(strcmp(field, "bob") == 0);                                  \
        LD_ASSERT(method(user, NULL));                                         \
        LD_ASSERT(field == NULL);                                              \
        LD_ASSERT(method(user, "shouldFree"));                                 \
        LD_ASSERT(strcmp(field, "shouldFree") == 0);                           \

    testStringField(LDUserSetIP, user->ip);
    testStringField(LDUserSetFirstName, user->firstName);
    testStringField(LDUserSetLastName, user->lastName);
    testStringField(LDUserSetEmail, user->email);
    testStringField(LDUserSetName, user->name);
    testStringField(LDUserSetAvatar, user->avatar);
    testStringField(LDUserSetCountry, user->country);
    testStringField(LDUserSetSecondary, user->secondary);

    #undef testStringField

    LD_ASSERT(user->privateAttributeNames == NULL);
    LD_ASSERT(attributes = LDNewArray());
    LD_ASSERT(tmp = LDNewText("name"));
    LD_ASSERT(LDArrayPush(attributes, tmp));
    LDUserSetPrivateAttributes(user, attributes);
    LD_ASSERT(user->privateAttributeNames == attributes);
    LD_ASSERT(attributes = LDNewArray());
    LD_ASSERT(tmp = LDNewText("email"));
    LD_ASSERT(LDArrayPush(attributes, tmp));
    LDUserSetPrivateAttributes(user, attributes);
    LD_ASSERT(user->privateAttributeNames == attributes);

    LD_ASSERT(user->custom == NULL);
    LD_ASSERT(attributes = LDNewObject());
    LD_ASSERT(tmp = LDNewText("bob"));
    LD_ASSERT(LDObjectSetKey(attributes, "otherName", tmp));
    LDUserSetCustomAttributesJSON(user, attributes);
    LD_ASSERT(user->custom == attributes);
    LD_ASSERT(attributes = LDNewObject());
    LD_ASSERT(tmp = LDNewText("alice"));
    LD_ASSERT(LDObjectSetKey(attributes, "otherName", tmp));
    LDUserSetCustomAttributesJSON(user, attributes);
    LD_ASSERT(user->custom == attributes);

    LDUserFree(user);
}

int
main(int argc, char **argv)
{
    LDBasicLoggerThreadSafeInitialize();
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLoggerThreadSafe);

    testBasicSerialization();
    testPrivateAttributes();
    testGeneratesKeyForNullKey();
    testSettersAndDefaults();

    LDBasicLoggerThreadSafeShutdown();

    return 0;
}
