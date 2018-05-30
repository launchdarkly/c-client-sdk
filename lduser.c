#include "ldapi.h"
#include "ldinternal.h"


LDUser *
LDUserNew(const char *key)
{
    LDUser *user;

    user = LDAlloc(sizeof(*user));
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
LDi_freeuser(LDUser *user)
{
    if (!user)
        return;
    LDFree(user->key);
    LDFree(user->secondary);
    LDFree(user->firstName);
    LDFree(user->lastName);
    LDFree(user->email);
    LDFree(user->name);
    LDFree(user->avatar);
    LDi_freehash(user->privateAttributeNames);
    LDFree(user);
}


cJSON *
LDi_usertojson(LDUser *lduser)
{
    cJSON *json;

    json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "key", lduser->key);
    if (lduser->anonymous)
        cJSON_AddBoolToObject(json, "anonymous", lduser->anonymous);
#define addstring(field) if (lduser->field) cJSON_AddStringToObject(json, #field, lduser->field)
    addstring(secondary);
    addstring(ip);
    addstring(firstName);
    addstring(lastName);
    addstring(email);
    addstring(name);
    addstring(avatar);
#undef addstring
    if (lduser->custom) {
        cJSON_AddItemToObject(json, "custom", LDi_hashtojson(lduser->custom));
    }
    return json;
}


void
LDUserAddPrivateAttribute(LDUser *user, const char *key)
{
    LDMapNode *node = LDAlloc(sizeof(*node));
    memset(node, 0, sizeof(*node));
    node->key = LDi_strdup(key);
    node->type = LDNodeBool;
    node->b = true;

    HASH_ADD_KEYPTR(hh, user->privateAttributeNames, node->key, strlen(node->key), node);
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