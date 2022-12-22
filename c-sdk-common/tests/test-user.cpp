#include "commonfixture.h"
#include "gtest/gtest.h"

extern "C" {
#include <string.h>

#include <launchdarkly/logging.h>
#include <launchdarkly/memory.h>
#include <launchdarkly/user.h>

#include "assertion.h"
#include "user.h"
}

class UserFixture : public CommonFixture {
};

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

TEST_F(UserFixture, ConstructNoSettings)
{
    struct LDUser *const user = LDi_userNew("abc");

    ASSERT_TRUE(user);

    LDUserFree(user);
}

TEST_F(UserFixture, ConstructAllSettings)
{
    struct LDUser *user;

    ASSERT_TRUE(user = constructBasic());

    LDUserFree(user);
}

TEST_F(UserFixture, SerializeEmptyEventUser)
{
    struct LDJSON *json;
    struct LDUser *user;
    char          *serialized;

    ASSERT_TRUE(user = LDi_userNew("abc"));
    ASSERT_TRUE(
        json = LDi_createEventUser(user, LDBooleanFalse, NULL));
    ASSERT_TRUE(serialized = LDJSONSerialize(json));

    ASSERT_STREQ(serialized, "{\"key\":\"abc\"}");

    LDUserFree(user);
    LDFree(serialized);
    LDJSONFree(json);
}

TEST_F(UserFixture, SerializeRedactedEventUser)
{
    struct LDJSON *json;
    struct LDUser *user;
    char          *serialized;
    struct LDJSON *custom, *child;

    ASSERT_TRUE(user = LDi_userNew("123"));

    ASSERT_TRUE(custom = LDNewObject());
    ASSERT_TRUE(child = LDNewNumber(42));
    ASSERT_TRUE(LDObjectSetKey(custom, "secret", child));
    ASSERT_TRUE(child = LDNewNumber(52));
    ASSERT_TRUE(LDObjectSetKey(custom, "notsecret", child));

    LDUserSetCustom(user, custom);
    ASSERT_TRUE(LDUserAddPrivateAttribute(user, "secret"));

    ASSERT_TRUE(
        json = LDi_createEventUser(user, LDBooleanFalse, NULL));
    ASSERT_TRUE(serialized = LDJSONSerialize(json));

    ASSERT_STREQ(
        serialized,
        "{\"key\":\"123\",\"custom\":"
        "{\"notsecret\":52},\"privateAttrs\":[\"secret\"]}");

    LDUserFree(user);
    LDFree(serialized);
    LDJSONFree(json);
}

TEST_F(UserFixture, SerializeAllEventUserAttributes)
{
    struct LDJSON *json;
    struct LDUser *user;
    char          *serialized;

    ASSERT_TRUE(user = constructBasic());

    ASSERT_TRUE(
        json = LDi_createEventUser(user, LDBooleanFalse, NULL));
    ASSERT_TRUE(serialized = LDJSONSerialize(json));

    ASSERT_STREQ(
        serialized,
        "{\"key\":\"abc\","
        "\"secondary\":\"unknown202\",\"ip\":\"127.0.0.1\","
        "\"firstName\":\"Jane\",\"lastName\":\"Doe\","
        "\"email\":\"janedoe@launchdarkly.com\","
        "\"name\":\"Jane\",\"avatar\":\"unknown101\",\"custom\":{}}");

    LDUserFree(user);
    LDFree(serialized);
    LDJSONFree(json);
}

TEST_F(UserFixture, SerializeBasicUser) {
    struct LDUser *user;
    char *json;

    ASSERT_TRUE(user = constructBasic());
    ASSERT_TRUE(json = LDi_serializeUser(user));

    ASSERT_STREQ(
        json,
        "{\"key\":\"abc\","
        "\"secondary\":\"unknown202\",\"ip\":\"127.0.0.1\","
        "\"firstName\":\"Jane\",\"lastName\":\"Doe\","
        "\"email\":\"janedoe@launchdarkly.com\","
        "\"name\":\"Jane\",\"avatar\":\"unknown101\","
        "\"privateAttributeNames\":[\"secret\"]}");

    LDUserFree(user);
    LDFree(json);
}

TEST_F(UserFixture, SerializeEmptyUser) {
    struct LDUser *user;
    char *json;

    ASSERT_TRUE(user = LDi_userNew("foo"));

    ASSERT_TRUE(json = LDi_serializeUser(user));

    ASSERT_STREQ(json,"{\"key\":\"foo\"}");

    LDUserFree(user);
    LDFree(json);
}

TEST_F(UserFixture, SerializeUserWithMultiplePrivateAttributes) {
    struct LDUser *user;
    char *json;

    ASSERT_TRUE(user = LDi_userNew("abc"));

    LDUserAddPrivateAttribute(user, "foo");
    LDUserAddPrivateAttribute(user, "bar");
    LDUserAddPrivateAttribute(user, "baz");

    ASSERT_TRUE(json = LDi_serializeUser(user));

    ASSERT_STREQ(json,"{\"key\":\"abc\",\"privateAttributeNames\":[\"foo\",\"bar\",\"baz\"]}");

    LDUserFree(user);
    LDFree(json);
}

TEST_F(UserFixture, SerializeUserWithMultipleCustomAttributes) {
    struct LDUser *user;
    struct LDJSON *custom;
    char *json;

    ASSERT_TRUE(custom = LDNewObject());

    ASSERT_TRUE(user = LDi_userNew("abc"));

    LDObjectSetKey(custom, "foo", LDNewBool(LDBooleanTrue));
    LDObjectSetKey(custom, "bar", LDNewBool(LDBooleanTrue));
    LDObjectSetKey(custom, "baz", LDNewBool(LDBooleanTrue));

    LDUserSetCustom(user, custom);

    ASSERT_TRUE(json = LDi_serializeUser(user));

    ASSERT_STREQ(json,"{\"key\":\"abc\",\"custom\":{\"foo\":true,\"bar\":true,\"baz\":true}}");

    LDUserFree(user);
    LDFree(json);
}

TEST_F(UserFixture, DefaultReplaceAndGet)
{
    struct LDUser *user;
    struct LDJSON *custom, *tmp, *tmp2, *tmp3, *attributes;

    ASSERT_TRUE(tmp2 = LDNewText("bob"));

    ASSERT_TRUE(user = LDi_userNew("bob"));
    ASSERT_TRUE(strcmp(user->key, "bob") == 0);
    ASSERT_TRUE(tmp = LDi_valueOfAttribute(user, "key"));
    ASSERT_TRUE(LDJSONCompare(tmp, tmp2));
    LDJSONFree(tmp);

    ASSERT_TRUE(user->anonymous == LDBooleanFalse);
    LDUserSetAnonymous(user, LDBooleanTrue);
    ASSERT_TRUE(user->anonymous == LDBooleanTrue);
    ASSERT_TRUE(tmp = LDi_valueOfAttribute(user, "anonymous"));
    ASSERT_TRUE(tmp3 = LDNewBool(LDBooleanTrue));
    ASSERT_TRUE(LDJSONCompare(tmp, tmp3));
    LDJSONFree(tmp);
    LDJSONFree(tmp3);

#define testStringField(method, field, fieldName)             \
    ASSERT_TRUE(field == NULL);                               \
    ASSERT_TRUE(method(user, "alice"));                       \
    ASSERT_STREQ(field, "alice");                 \
    ASSERT_TRUE(method(user, "bob"));                         \
    ASSERT_STREQ(field, "bob");                   \
    ASSERT_TRUE(tmp = LDi_valueOfAttribute(user, fieldName)); \
    ASSERT_TRUE(LDJSONCompare(tmp, tmp2));                    \
    LDJSONFree(tmp);                                          \
    ASSERT_TRUE(method(user, NULL));                          \
    ASSERT_EQ(field, nullptr);

    testStringField(LDUserSetIP, user->ip, "ip");
    testStringField(LDUserSetFirstName, user->firstName, "firstName");
    testStringField(LDUserSetLastName, user->lastName, "lastName");
    testStringField(LDUserSetEmail, user->email, "email");
    testStringField(LDUserSetName, user->name, "name");
    testStringField(LDUserSetAvatar, user->avatar, "avatar");
    testStringField(LDUserSetCountry, user->country, "country");
    testStringField(LDUserSetSecondary, user->secondary, "secondary");

#undef testStringField

    ASSERT_TRUE(user->custom == NULL);
    ASSERT_TRUE(custom = LDNewObject());
    LDUserSetCustom(user, custom);
    ASSERT_TRUE(user->custom == custom);
    ASSERT_TRUE(custom = LDNewObject());
    LDUserSetCustom(user, custom);
    ASSERT_TRUE(tmp3 = LDNewNumber(52));
    ASSERT_TRUE(LDObjectSetKey(custom, "count", tmp3));
    ASSERT_TRUE(user->custom == custom);
    ASSERT_TRUE(tmp = LDi_valueOfAttribute(user, "count"));
    ASSERT_TRUE(LDJSONCompare(tmp, tmp3));
    LDJSONFree(tmp);
    ASSERT_EQ(LDi_valueOfAttribute(user, "unknown"), nullptr);
    LDUserSetCustom(user, NULL);
    ASSERT_EQ(user->custom, nullptr);
    ASSERT_EQ(LDi_valueOfAttribute(user, "unknown"), nullptr);

    ASSERT_TRUE(attributes = LDNewArray());
    ASSERT_TRUE(LDJSONCompare(user->privateAttributeNames, attributes));
    ASSERT_TRUE(tmp = LDNewText("name"));
    ASSERT_TRUE(LDArrayPush(attributes, tmp));
    LDUserAddPrivateAttribute(user, "name");
    ASSERT_TRUE(LDJSONCompare(user->privateAttributeNames, attributes));

    LDJSONFree(tmp2);
    LDJSONFree(attributes);
    LDUserFree(user);
}
