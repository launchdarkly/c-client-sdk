#include "ldapi.h"
#include "ldinternal.h"

struct LDUser *
LDUserNew(const char *const key)
{
    struct LDUser *user;

    LDi_once(&LDi_earlyonce, LDi_earlyinit);

    if (!(user = LDAlloc(sizeof(*user)))) {
        return NULL;
    }

    memset(user, 0, sizeof(*user));

    if (!key || key[0] == '\0') {
        char *const deviceid = LDi_deviceid();
        if (deviceid) {
            LDSetString(&user->key, deviceid);
            LDFree(deviceid);
        } else {
            LD_LOG(LD_LOG_WARNING,
                "Failed to get device ID falling back to random ID");
            char randomkey[32 + 1] = { 0 };
            LDi_randomhex(randomkey, sizeof(randomkey) - 1);
            LDSetString(&user->key, randomkey);
        }
        LD_LOG_1(LD_LOG_INFO, "Setting user key to: %s", user->key);
        user->anonymous = true;
    }
    else {
        LDSetString(&user->key, key);
        user->anonymous = false;
    }

    user->secondary             = NULL;
    user->ip                    = NULL;
    user->firstName             = NULL;
    user->lastName              = NULL;
    user->email                 = NULL;
    user->name                  = NULL;
    user->avatar                = NULL;
    user->privateAttributeNames = NULL;
    user->custom                = NULL;

    return user;
}

void
LDUserFree(struct LDUser *const user)
{
    if (user) {
        LDFree(user->key);
        LDFree(user->secondary);
        LDFree(user->firstName);
        LDFree(user->lastName);
        LDFree(user->email);
        LDFree(user->name);
        LDFree(user->avatar);
        LDJSONFree(user->custom);
        LDJSONFree(user->privateAttributeNames);
        LDFree(user);
    }
}

bool
LDi_textInArray(const struct LDJSON *const array, const char *const text)
{
    struct LDJSON *iter;

    LD_ASSERT(text);

    if (array == NULL) {
        return false;
    }

    for (iter = LDGetIter(array); iter; iter = LDIterNext(iter)) {
        if (strcmp(LDGetText(iter), text) == 0) {
            return true;
        }
    }

    return false;
}

static bool
isPrivateAttr(
    const struct LDConfig *const config,
    const struct LDUser *const   user,
    const char *const            key
) {
    bool global = false;

    if (config) {
        global = config->allAttributesPrivate ||
            LDi_textInArray(config->privateAttributeNames, key);
    }

    return global || LDi_textInArray(user->privateAttributeNames, key);
}

static bool
addHidden(struct LDJSON **const ref, const char *const value){
    struct LDJSON *text;

    LD_ASSERT(ref);
    LD_ASSERT(value);

    text = NULL;

    if (!(*ref)) {
        *ref = LDNewArray();

        if (!(*ref)) {
            return false;
        }
    }

    if (!(text = LDNewText(value))) {
        return false;
    }

    LDArrayPush(*ref, text);

    return true;
}

struct LDJSON *
LDi_userToJSON(
    const struct LDConfig *const config,
    const struct LDUser *const   lduser,
    const bool                   redact
) {
    struct LDJSON *hidden, *json, *temp;

    LD_ASSERT(lduser);

    hidden = NULL;
    json   = NULL;
    temp   = NULL;

    if (!(json = LDNewObject())) {
        return NULL;
    }

    if (!(temp = LDNewText(lduser->key))) {
        LDJSONFree(json);

        return NULL;
    }

    if (!LDObjectSetKey(json, "key", temp)) {
        LDJSONFree(temp);
        LDJSONFree(json);

        return NULL;
    }

    if (lduser->anonymous) {
        if (!(temp = LDNewBool(lduser->anonymous))) {
            LDJSONFree(json);

            return NULL;
        }

        if (!LDObjectSetKey(json, "anonymous", temp)) {
            LDJSONFree(temp);
            LDJSONFree(json);

            return NULL;
        }
    }

    #define addstring(field)                                                   \
        if (lduser->field) {                                                   \
            if (redact && isPrivateAttr(config, lduser, #field)) {             \
                if (!addHidden(&hidden, #field)) {                             \
                    LDJSONFree(json);                                          \
                                                                               \
                    return NULL;                                               \
                }                                                              \
            }                                                                  \
            else {                                                             \
                if (!(temp = LDNewText(lduser->field))) {                      \
                    LDJSONFree(json);                                          \
                                                                               \
                    return NULL;                                               \
                }                                                              \
                                                                               \
                if (!LDObjectSetKey(json, #field, temp)) {                     \
                    LDJSONFree(json);                                          \
                                                                               \
                    return NULL;                                               \
                }                                                              \
            }                                                                  \
        }                                                                      \

    addstring(secondary);
    addstring(ip);
    addstring(firstName);
    addstring(lastName);
    addstring(email);
    addstring(name);
    addstring(avatar);

    if (lduser->custom) {
        struct LDJSON *const custom = LDJSONDuplicate(lduser->custom);

        if (!custom) {
            LDJSONFree(json);

            return NULL;
        }

        if (redact && LDJSONGetType(custom) == LDObject) {
            struct LDJSON *item = LDGetIter(custom);

            while(item) {
                /* must record next to make delete safe */
                struct LDJSON *const next = LDIterNext(item);

                if (isPrivateAttr(config, lduser, LDIterKey(item))) {
                    if (!addHidden(&hidden, LDIterKey(item))) {
                        LDJSONFree(json);
                        LDJSONFree(custom);

                        return NULL;
                    }

                    LDObjectDeleteKey(custom, LDIterKey(item));
                }

                item = next;
            }
        }

        if (!LDObjectSetKey(json, "custom", custom)) {
            LDJSONFree(custom);
            LDJSONFree(json);

            return NULL;
        }
    }

    if (hidden) {
        if (!LDObjectSetKey(json, "privateAttrs", hidden)) {
            LDJSONFree(json);

            return NULL;
        }
    }

    return json;

    #undef addstring
}

void
LDUserSetCustomAttributesJSON(struct LDUser *const user,
    struct LDJSON *const custom)
{
    LD_ASSERT(user);

    if (user->custom) {
        LDJSONFree(user->custom);
    }

    user->custom = custom;
}

void
LDUserSetPrivateAttributes(struct LDUser *const user,
    struct LDJSON *const privateAttributes)
{
    LD_ASSERT(user);
    LD_ASSERT(LDJSONGetType(privateAttributes) == LDArray);

    if (user->privateAttributeNames) {
        LDJSONFree(user->privateAttributeNames);
    }

    user->privateAttributeNames = privateAttributes;
}

void
LDUserSetAnonymous(struct LDUser *const user, const LDBoolean anon)
{
    LD_ASSERT(user);

    user->anonymous = anon;
}

void
LDUserSetIP(struct LDUser *const user, const char *const str)
{
    LD_ASSERT(user);

    LD_ASSERT(LDSetString(&user->ip, str));
}

void
LDUserSetFirstName(struct LDUser *const user, const char *const str)
{
    LD_ASSERT(user);

    LD_ASSERT(LDSetString(&user->firstName, str));
}

void
LDUserSetLastName(struct LDUser *const user, const char *const str)
{
    LD_ASSERT(user);

    LD_ASSERT(LDSetString(&user->lastName, str));
}

void
LDUserSetEmail(struct LDUser *const user, const char *const str)
{
    LD_ASSERT(user);

    LD_ASSERT(LDSetString(&user->email, str));
}

void
LDUserSetName(struct LDUser *const user, const char *const str)
{
    LD_ASSERT(user);

    LDSetString(&user->name, str);
}

void
LDUserSetAvatar(struct LDUser *const user, const char *const str)
{
    LD_ASSERT(user);

    LD_ASSERT(LDSetString(&user->avatar, str));
}

void
LDUserSetSecondary(struct LDUser *const user, const char *const str)
{
    LD_ASSERT(user);

    LD_ASSERT(LDSetString(&user->secondary, str));
}

char *
LDi_usertojsontext(const struct LDClient *const client,
    const struct LDUser *const user, const bool redact)
{
    struct LDJSON *userJSON;
    char *userText;

    userJSON = LDi_userToJSON(client->shared->sharedConfig, user, redact);

    if (!userJSON) {
        LD_LOG(LD_LOG_ERROR, "LDi_usertojson failed in LDi_usertojsontext");

        return NULL;
    }

    userText = LDJSONSerialize(userJSON);

    LDJSONFree(userJSON);

    if (!userText) {
        LD_LOG(LD_LOG_ERROR, "LDJSONSerialize failed in LDi_usertojsontext");

        return NULL;
    }

    return userText;
}
