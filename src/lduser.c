#include "ldapi.h"
#include "ldinternal.h"

LDUser *
LDUserNew(const char *const key)
{
    LDi_once(&LDi_earlyonce, LDi_earlyinit);

    LDUser *const user = LDAlloc(sizeof(*user)); LD_ASSERT(user != NULL);
    memset(user, 0, sizeof(*user));

    if (!key || key[0] == '\0') {
        char *const deviceid = LDi_deviceid();
        if (deviceid) {
            LDSetString(&user->key, deviceid);
            LDFree(deviceid);
        } else {
            LDi_log(LD_LOG_WARNING, "Failed to get device ID falling back to random ID");
            char randomkey[32 + 1] = { 0 };
            LDi_randomhex(randomkey, sizeof(randomkey) - 1);
            LDSetString(&user->key, randomkey);
        }
        LDi_log(LD_LOG_INFO, "Setting user key to: %s", user->key);
        user->anonymous = true;
    }
    else {
        LDSetString(&user->key, key);
        user->anonymous = false;
    }

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
LDUserFree(LDUser *const user)
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
isPrivateAttr(LDClient *const client, LDUser *const user, const char *const key)
{
    bool global = false;

    if (client) {
        global = client->shared->sharedConfig->allAttributesPrivate  ||
            (LDNodeLookup(client->shared->sharedConfig->privateAttributeNames, key) != NULL);
    }

    return global || (LDNodeLookup(user->privateAttributeNames, key) != NULL);
}

static void
addHidden(cJSON **ref, const char *const value){
    if (!(*ref)) { *ref = cJSON_CreateArray(); }
    cJSON_AddItemToArray(*ref, cJSON_CreateString(value));
}

cJSON *
LDi_usertojson(LDClient *const client, LDUser *const lduser, const bool redact)
{
    cJSON *const json = cJSON_CreateObject();

    cJSON_AddStringToObject(json, "key", lduser->key);

    if (lduser->anonymous) {
        cJSON_AddBoolToObject(json, "anonymous", lduser->anonymous);
    }

    cJSON *hidden = NULL;

    #define addstring(field)                                                   \
        if (lduser->field) {                                                   \
            if (redact && isPrivateAttr(client, lduser, #field)) {             \
                addHidden(&hidden, #field);                                    \
            }                                                                  \
            else {                                                             \
                cJSON_AddStringToObject(json, #field, lduser->field);          \
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
        cJSON *const custom = LDi_hashtojson(lduser->custom);
        if (redact && cJSON_IsObject(custom)) {
            for (cJSON *item = custom->child; item;) {
                cJSON *const next = item->next; //must record next to make delete safe
                if (isPrivateAttr(client, lduser, item->string)) {
                    addHidden(&hidden, item->string);
                    cJSON *const current = cJSON_DetachItemFromObjectCaseSensitive(custom, item->string);
                    cJSON_Delete(current);
                }
                item = next;
            }
        }
        cJSON_AddItemToObject(json, "custom", custom);
    }

    if (hidden) {
        cJSON_AddItemToObject(json, "privateAttrs", hidden);
    }

    return json;

    #undef addstring
    #undef addhidden
}

void
LDUserAddPrivateAttribute(LDUser *const user, const char *const key)
{
    LD_ASSERT(user); LD_ASSERT(key);

    LDNode *const node = LDAlloc(sizeof(*node)); LD_ASSERT(node);
    memset(node, 0, sizeof(*node));

    node->key = LDi_strdup(key);
    node->type = LDNodeBool;
    node->b = true;
    LD_ASSERT(node->key);

    HASH_ADD_KEYPTR(hh, user->privateAttributeNames, node->key, strlen(node->key), node);
}

bool
LDUserSetCustomAttributesJSON(LDUser *const user, const char *const jstring)
{
    LD_ASSERT(user);

    if (jstring) {
        cJSON *const json = cJSON_Parse(jstring);
        if (!json) {
            return false;
        }
        user->custom = LDi_jsontohash(json, 0);
        cJSON_Delete(json);
    }
    else {
        LDi_freehash(user->custom);
        user->custom = NULL;
    }

    return true;
}

void
LDUserSetCustomAttributes(LDUser *const user, LDNode *const custom)
{
    LD_ASSERT(user);
    LDi_freehash(user->custom);
    user->custom = custom;
}

void
LDUserSetAnonymous(LDUser *const user, const bool anon)
{
    LD_ASSERT(user); user->anonymous = anon;
}

void
LDUserSetIP(LDUser *const user, const char *const str)
{
    LD_ASSERT(user); LD_ASSERT(LDSetString(&user->ip, str));
}

void
LDUserSetFirstName(LDUser *const user, const char *const str)
{
    LD_ASSERT(user); LD_ASSERT(LDSetString(&user->firstName, str));
}

void
LDUserSetLastName(LDUser *const user, const char *const str)
{
    LD_ASSERT(user); LD_ASSERT(LDSetString(&user->lastName, str));
}

void
LDUserSetEmail(LDUser *const user, const char *const str)
{
    LD_ASSERT(user); LD_ASSERT(LDSetString(&user->email, str));
}

void
LDUserSetName(LDUser *const user, const char *const str)
{
    LD_ASSERT(user); LDSetString(&user->name, str);
}

void
LDUserSetAvatar(LDUser *const user, const char *const str)
{
    LD_ASSERT(user); LD_ASSERT(LDSetString(&user->avatar, str));
}

void
LDUserSetSecondary(LDUser *const user, const char *const str)
{
    LD_ASSERT(user); LD_ASSERT(LDSetString(&user->secondary, str));
}
