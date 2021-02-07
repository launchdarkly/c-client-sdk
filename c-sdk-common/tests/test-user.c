#include <stdio.h>
#include <string.h>

#include <launchdarkly/memory.h>
#include <launchdarkly/logging.h>
#include <launchdarkly/user.h>

#include "assertion.h"
#include "user.h"
#include "utility.h"

static struct LDUser *
constructBasic(void)
{
    struct LDJSON *custom;
    struct LDUser *user;

    LD_ASSERT(user = LDi_userNew("abc"));

    LDUserSetAnonymous(user, LDBooleanFalse);
    LD_ASSERT(LDUserSetIP(user, "127.0.0.1"));
    LD_ASSERT(LDUserSetFirstName(user, "Jane"));
    LD_ASSERT(LDUserSetLastName(user, "Doe"));
    LD_ASSERT(LDUserSetEmail(user, "janedoe@launchdarkly.com"));
    LD_ASSERT(LDUserSetName(user, "Jane"));
    LD_ASSERT(LDUserSetAvatar(user, "unknown101"));
    LD_ASSERT(LDUserSetSecondary(user, "unknown202"));

    LD_ASSERT(custom = LDNewObject());

    LDUserSetCustom(user, custom);

    LD_ASSERT(LDUserAddPrivateAttribute(user, "secret"));

    return user;
}

static void
constructNoSettings(void)
{
    struct LDUser *const user = LDi_userNew("abc");

    LD_ASSERT(user);

    LDUserFree(user);
}

static void
constructAllSettings(void)
{
    struct LDUser *user;

    LD_ASSERT(user = constructBasic());

    LDUserFree(user);
}

static void
serializeEmpty(void)
{
    struct LDJSON *json;
    struct LDUser *user;
    char *serialized;

    LD_ASSERT(user = LDi_userNew("abc"));
    LD_ASSERT(json = LDi_userToJSON(
        user, LDBooleanFalse, LDBooleanFalse, NULL));
    LD_ASSERT(serialized = LDJSONSerialize(json));

    LD_ASSERT(strcmp(serialized, "{\"key\":\"abc\"}") == 0);

    LDUserFree(user);
    LDFree(serialized);
    LDJSONFree(json);
}

static void
serializeRedacted(void)
{
    struct LDJSON *json;
    struct LDUser *user;
    char *serialized;
    struct LDJSON *custom, *child;

    LD_ASSERT(user = LDi_userNew("123"));

    LD_ASSERT(custom = LDNewObject());
    LD_ASSERT(child = LDNewNumber(42));
    LD_ASSERT(LDObjectSetKey(custom, "secret", child));
    LD_ASSERT(child = LDNewNumber(52));
    LD_ASSERT(LDObjectSetKey(custom, "notsecret", child));

    LDUserSetCustom(user, custom);
    LD_ASSERT(LDUserAddPrivateAttribute(user, "secret"));

    LD_ASSERT(json = LDi_userToJSON(user, LDBooleanTrue, LDBooleanFalse, NULL));
    LD_ASSERT(serialized = LDJSONSerialize(json));

    LD_ASSERT(strcmp(serialized, "{\"key\":\"123\",\"custom\":"
      "{\"notsecret\":52},\"privateAttrs\":[\"secret\"]}") == 0);

    LDUserFree(user);
    LDFree(serialized);
    LDJSONFree(json);
}

static void
serializeAll(void)
{
    struct LDJSON *json;
    struct LDUser *user;
    char *serialized;

    LD_ASSERT(user = constructBasic());

    LD_ASSERT(json = LDi_userToJSON(
        user, LDBooleanFalse, LDBooleanFalse, NULL));
    LD_ASSERT(serialized = LDJSONSerialize(json));

    LD_ASSERT(strcmp(serialized, "{\"key\":\"abc\","
      "\"secondary\":\"unknown202\",\"ip\":\"127.0.0.1\","
      "\"firstName\":\"Jane\",\"lastName\":\"Doe\","
      "\"email\":\"janedoe@launchdarkly.com\","
      "\"name\":\"Jane\",\"avatar\":\"unknown101\",\"custom\":{}}") == 0);

    LDUserFree(user);
    LDFree(serialized);
    LDJSONFree(json);
}

static void
testDefaultReplaceAndGet(void)
{
    struct LDUser *user;
    struct LDJSON *custom, *tmp, *tmp2, *tmp3, *attributes;

    LD_ASSERT(tmp2 = LDNewText("bob"));

    LD_ASSERT(user = LDi_userNew("bob"));
    LD_ASSERT(strcmp(user->key, "bob") == 0);
    LD_ASSERT(tmp = LDi_valueOfAttribute(user, "key"));
    LD_ASSERT(LDJSONCompare(tmp, tmp2));
    LDJSONFree(tmp);

    LD_ASSERT(user->anonymous == LDBooleanFalse);
    LDUserSetAnonymous(user, LDBooleanTrue);
    LD_ASSERT(user->anonymous == LDBooleanTrue);
    LD_ASSERT(tmp = LDi_valueOfAttribute(user, "anonymous"));
    LD_ASSERT(tmp3 = LDNewBool(LDBooleanTrue));
    LD_ASSERT(LDJSONCompare(tmp, tmp3));
    LDJSONFree(tmp);
    LDJSONFree(tmp3);
    
    #define testStringField(method, field, fieldName)                          \
        LD_ASSERT(field == NULL);                                              \
        LD_ASSERT(method(user, "alice"));                                      \
        LD_ASSERT(strcmp(field, "alice") == 0);                                \
        LD_ASSERT(method(user, "bob"));                                        \
        LD_ASSERT(strcmp(field, "bob") == 0);                                  \
        LD_ASSERT(tmp = LDi_valueOfAttribute(user, fieldName))                 \
        LD_ASSERT(LDJSONCompare(tmp, tmp2));                                   \
        LDJSONFree(tmp);                                                       \
        LD_ASSERT(method(user, NULL));                                         \
        LD_ASSERT(field == NULL);

    testStringField(LDUserSetIP, user->ip, "ip");
    testStringField(LDUserSetFirstName, user->firstName, "firstName");
    testStringField(LDUserSetLastName, user->lastName, "lastName");
    testStringField(LDUserSetEmail, user->email, "email");
    testStringField(LDUserSetName, user->name, "name");
    testStringField(LDUserSetAvatar, user->avatar, "avatar");
    testStringField(LDUserSetCountry, user->country, "country");
    testStringField(LDUserSetSecondary, user->secondary, "secondary");

    #undef testStringField

    LD_ASSERT(user->custom == NULL);
    LD_ASSERT(custom = LDNewObject());
    LDUserSetCustom(user, custom);
    LD_ASSERT(user->custom == custom);
    LD_ASSERT(custom = LDNewObject());
    LDUserSetCustom(user, custom);
    LD_ASSERT(tmp3 = LDNewNumber(52));
    LD_ASSERT(LDObjectSetKey(custom, "count", tmp3));
    LD_ASSERT(user->custom == custom);
    LD_ASSERT(tmp = LDi_valueOfAttribute(user, "count"));
    LD_ASSERT(LDJSONCompare(tmp, tmp3));
    LDJSONFree(tmp);
    LD_ASSERT(LDi_valueOfAttribute(user, "unknown") == NULL);
    LDUserSetCustom(user, NULL);
    LD_ASSERT(user->custom == NULL);
    LD_ASSERT(LDi_valueOfAttribute(user, "unknown") == NULL);

    LD_ASSERT(attributes = LDNewArray());
    LD_ASSERT(LDJSONCompare(user->privateAttributeNames, attributes));
    LD_ASSERT(tmp = LDNewText("name"));
    LD_ASSERT(LDArrayPush(attributes, tmp));
    LDUserAddPrivateAttribute(user, "name");
    LD_ASSERT(LDJSONCompare(user->privateAttributeNames, attributes));

    LDJSONFree(tmp2);
    LDJSONFree(attributes);
    LDUserFree(user);
}

int
main()
{
    LDBasicLoggerThreadSafeInitialize();
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLoggerThreadSafe);
    LDGlobalInit();

    constructNoSettings();
    constructAllSettings();
    serializeEmpty();
    serializeRedacted();
    serializeAll();
    testDefaultReplaceAndGet();

    LDBasicLoggerThreadSafeShutdown();

    return 0;
}
