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
isPrivateAttr(LDClient *const client, LDUser *const user, const char *const key)
{
    bool global = false;

    if (client) {
        global = client->config->allAttributesPrivate  ||
            (LDNodeLookup(client->config->privateAttributeNames, key) != NULL);
    }

    return global || (LDNodeLookup(user->privateAttributeNames, key) != NULL);
}

//checks for duplicates because user may set field in custom
static void
addHidden(cJSON **ref, const char *const value){
    if (!(*ref)) { *ref = cJSON_CreateArray(); }
    cJSON *const hidden = *ref; bool exists = false;
    for (cJSON *item = hidden->child; item; item = item->next) {
        if (strcmp(item->valuestring, value) == 0) {
            exists = true; break;
        }
    }
    if (!exists) {
        cJSON_AddItemToArray(hidden, cJSON_CreateString(value));
    }
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

    #define addstring(field)                                                     \
        if (lduser->field) {                                                    \
            if (redact && isPrivateAttr(client, lduser, #field)) {              \
                addHidden(&hidden, #field);                                     \
            }                                                                  \
            else {                                                             \
                cJSON_AddStringToObject(json, #field, lduser->field);            \
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
        if (redact && isPrivateAttr(client, lduser, "custom")) {
            addHidden(&hidden, "custom");
        }
        else {
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
    }

    if (hidden) {
        cJSON_AddItemToObject(json, "privateAttrs", hidden);
    }

    return json;

    #undef addstring
    #undef addhidden
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
