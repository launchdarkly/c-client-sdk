#include "ldapi.h"
#include "ldinternal.h"


LDUser *
LDUserNew(const char *const key)
{
    LDi_once(&LDi_earlyonce, LDi_earlyinit);

    LDUser *const user = LDAlloc(sizeof(*user));
    if (!user) {
        LDi_log(2, "no memory for user\n");
        return NULL;
    }
    memset(user, 0, sizeof(*user));

    LDSetString(&user->key, key);
    user->anonymous = false;
    user->secondary = NULL;
    user->ip = NULL;
    user->firstName = NULL;
    user->lastName = NULL;
    user->email = NULL;
    user->name = NULL;
    user->avatar = NULL;
    user->custom = NULL;
    user->privateAttributeNames = NULL;

    return user;
}

void
LDi_freeuser(LDUser *const user)
{
    if (!user) { return; }
    LDFree(user->key);
    LDFree(user->secondary);
    LDFree(user->firstName);
    LDFree(user->lastName);
    LDFree(user->email);
    LDFree(user->name);
    LDFree(user->avatar);
    LDi_freehash(user->custom);
    LDi_freehash(user->privateAttributeNames);
    LDFree(user);
}

static bool
isPrivateAttr(LDClient *const client, const char *const key)
{
    return client->config->allAttributesPrivate ||
        (LDNodeLookup(client->config->privateAttributeNames, key) != NULL) ||
        (LDNodeLookup(client->user->privateAttributeNames, key) != NULL);
}

cJSON *
LDi_usertojson(LDClient *const client, LDUser *lduser)
{
    cJSON *const json = cJSON_CreateObject();

    cJSON_AddStringToObject(json, "key", lduser->key);

    if (lduser->anonymous) {
        cJSON_AddBoolToObject(json, "anonymous", lduser->anonymous);
    }

    cJSON *const hidden = cJSON_CreateArray();

  #define addstring(field)                                                       \
    if (lduser->field) {                                                        \
        if (isPrivateAttr(client, #field)) {                                    \
            cJSON_AddItemToArray(hidden, cJSON_CreateString(#field));           \
        }                                                                      \
        else {                                                                 \
            cJSON_AddStringToObject(json, #field, lduser->field);                \
        }                                                                      \
    }                                                                          \

    addstring(secondary);
    addstring(ip);
    addstring(firstName);
    addstring(lastName);
    addstring(email);
    addstring(name);
    addstring(avatar);
  #undef addstring

    if (lduser->custom) {
        if (isPrivateAttr(client, "custom")) {
            cJSON_AddItemToArray(hidden, cJSON_CreateString("custom"));
        }
        else {
            cJSON_AddItemToObject(json, "custom", LDi_hashtojson(lduser->custom));
        }
    }

    if (cJSON_GetArraySize(hidden)) {
        cJSON_AddItemToObject(json, "privateAttrs", hidden);
    } else {
        cJSON_Delete(hidden);
    }

    return json;
}

bool
LDUserAddPrivateAttribute(LDUser *const user, const char *const key)
{
    if (!user) {
        LDi_log(2, "Passed NULL user to LDUserAddPrivateAttribute\n");
        return false;
    }

    if (!key) {
        LDi_log(2, "Passed NULL attribute key to LDUserAddPrivateAttribute\n");
        return false;
    }

    LDNode *const node = LDAlloc(sizeof(*node));

    if (!node) {
        LDi_log(2, "LDAlloc failed in LDUserAddPrivateAttribute\n");
        return false;
    }

    memset(node, 0, sizeof(*node));
    node->key = LDi_strdup(key);
    node->type = LDNodeBool;
    node->b = true;

    HASH_ADD_KEYPTR(hh, user->privateAttributeNames, node->key, strlen(node->key), node);

    return true;
}

bool
LDUserSetCustomAttributesJSON(LDUser *user, const char *jstring)
{
    cJSON *json = cJSON_Parse(jstring);
    if (!json) {
        return false;
    }
    user->custom = LDi_jsontohash(json, 0);
    cJSON_Delete(json);

    return true;
}

void
LDUSerSetCustomAttributes(LDUser *user, LDNode *custom)
{
    user->custom = custom;
}

void
LDUserSetAnonymous(LDUser *user, bool anon)
{
    user->anonymous = anon;
}

void
LDUserSetIP(LDUser *user, const char *str)
{
    LDSetString(&user->ip, str);
}

void
LDUserSetFirstName(LDUser *user, const char *str)
{
    LDSetString(&user->firstName, str);
}

void
LDUserSetLastName(LDUser *user, const char *str)
{
    LDSetString(&user->lastName, str);
}

void
LDUserSetEmail(LDUser *user, const char *str)
{
    LDSetString(&user->email, str);
}

void
LDUserSetName(LDUser *user, const char *str)
{
    LDSetString(&user->name, str);
}

void
LDUserSetAvatar(LDUser *user, const char *str)
{
    LDSetString(&user->avatar, str);
}

void
LDUserSetSecondary(LDUser *user, const char *str)
{
    LDSetString(&user->secondary, str);
}
