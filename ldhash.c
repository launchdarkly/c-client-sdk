#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ldapi.h"
#include "ldinternal.h"

LDNode *
LDNodeCreateHash()
{
    /* empty hash is represented by NULL pointer */
    return NULL;
}

static LDNode *
newnode(LDNode **const hash, const char *const key, const LDNodeType type)
{
    LDNode *node = LDAlloc(sizeof(*node));
    memset(node, 0, sizeof(*node));
    node->key = LDi_strdup(key);
    node->type = type;
    HASH_ADD_KEYPTR(hh, *hash, node->key, strlen(node->key), node);
    return node;
}

LDNode *
LDNodeAddNone(LDNode **const hash, const char *const key)
{
    return newnode(hash, key, LDNodeNone);
}

LDNode *
LDNodeAddBool(LDNode **const hash, const char *const key, const bool b)
{
    LDNode *const node = newnode(hash, key, LDNodeBool);
    node->b = b;
    return node;
}

LDNode *
LDNodeAddNumber(LDNode **const hash, const char *const key, const double n)
{
    LDNode *const node = newnode(hash, key, LDNodeNumber);
    node->n = n;
    return node;
}

LDNode *
LDNodeAddString(LDNode **const hash, const char *const key, const char *const s)
{
    LDNode *const node = newnode(hash, key, LDNodeString);
    node->s = LDi_strdup(s);
    return node;
}

LDNode *
LDNodeAddHash(LDNode **const hash, const char *const key, LDNode *const h)
{
    LDNode *const node = newnode(hash, key, LDNodeHash);
    node->h = h;
    return node;
}

LDNode *
LDNodeAddArray(LDNode **const hash, const char *const key, LDNode *const a)
{
    LDNode *const node = newnode(hash, key, LDNodeArray);
    node->a = a;
    return node;
}

LDNode *
LDNodeCreateArray()
{
    /* same thing as map actually */
    return NULL;
}

static LDNode *
appendnode(LDNode **const array, const LDNodeType type)
{
    LDNode *const node = LDAlloc(sizeof(*node));
    memset(node, 0, sizeof(*node));
    node->idx = HASH_COUNT(*array);
    node->type = type;
    HASH_ADD_INT(*array, idx, node);
    return node;
}

LDNode *
LDNodeAppendBool(LDNode **const array, const bool b)
{
    LDNode *const node = appendnode(array, LDNodeBool);
    node->b = b;
    return node;
}

LDNode *
LDNodeAppendNumber(LDNode **const array, const double n)
{
    LDNode *const node = appendnode(array, LDNodeNumber);
    node->n = n;
    return node;
}

LDNode *
LDNodeAppendString(LDNode **const array, const char *const s)
{
    LDNode *const node = appendnode(array, LDNodeString);
    node->s = LDi_strdup(s);
    return node;
}

LDNode *
LDNodeLookup(const LDNode *const hash, const char *const key)
{
    LDNode *res = NULL;
    if (hash) {
        HASH_FIND_STR(hash, key, res);
    }
    return res;
}

LDNode *
LDNodeIndex(const LDNode *const array, const unsigned int idx)
{
    LDNode *res = NULL;
    HASH_FIND_INT(array, &idx, res);
    return res;
}

unsigned int
LDNodeCount(const LDNode *const hash)
{
    return HASH_COUNT(hash);
}

void
LDNodeFree(LDNode **const hash)
{
    LDi_freehash(*hash);
    *hash = NULL;
}

cJSON *
LDi_hashtoversionedjson(const LDNode *const hash)
{
    cJSON *const json = cJSON_CreateObject();

    for (const LDNode *node = hash; node; node=node->hh.next) {
        cJSON *const val = cJSON_CreateObject();

        switch (node->type) {
        case LDNodeNone:
            cJSON_AddNullToObject(val, "value");
            break;
        case LDNodeBool:
            cJSON_AddBoolToObject(val, "value", (int)node->b);
            break;
        case LDNodeNumber:
            cJSON_AddNumberToObject(val, "value", node->n);
            break;
        case LDNodeString:
            cJSON_AddStringToObject(val, "value", node->s);
            break;
        case LDNodeHash:
            cJSON_AddItemToObject(val, "value", LDi_hashtojson(node->h));
            break;
        case LDNodeArray:
            cJSON_AddItemToObject(val, "value", LDi_arraytojson(node->a));
            break;
        case LDNodeNone:
            break;
        }

        if (node->version) {
            cJSON_AddNumberToObject(val, "version", node->version);
        }

        if (node->variation) {
            cJSON_AddNumberToObject(val, "variation", node->variation);
        }

        if (node->flagversion) {
            cJSON_AddNumberToObject(val, "flagVersion", node->flagversion);
        }

        cJSON_AddItemToObject(json, node->key, val);
    }

    return json;
}

cJSON *
LDi_hashtojson(const LDNode *const hash)
{
    cJSON *const json = cJSON_CreateObject();

    for (const LDNode *node = hash; node; node=node->hh.next) {
        switch (node->type) {
        case LDNodeNone:
            cJSON_AddNullToObject(json, node->key);
            break;
        case LDNodeBool:
            cJSON_AddBoolToObject(json, node->key, (int)node->b);
            break;
        case LDNodeNumber:
            cJSON_AddNumberToObject(json, node->key, node->n);
            break;
        case LDNodeString:
            cJSON_AddStringToObject(json, node->key, node->s);
            break;
        case LDNodeHash:
            cJSON_AddItemToObject(json, node->key, LDi_hashtojson(node->h));
            break;
        case LDNodeArray:
            cJSON_AddItemToObject(json, node->key, LDi_arraytojson(node->a));
            break;
        case LDNodeNone:
            break;
        }
    }

    return json;
}

cJSON *
LDi_arraytojson(const LDNode *const hash)
{
    cJSON *const json = cJSON_CreateArray();

    for (const LDNode *node = hash; node; node=node->hh.next) {
        switch (node->type) {
        case LDNodeNone:
            cJSON_AddItemToArray(json, cJSON_CreateNull());
            break;
        case LDNodeBool:
            cJSON_AddItemToArray(json, cJSON_CreateBool((int)node->b));
            break;
        case LDNodeNumber:
            cJSON_AddItemToArray(json, cJSON_CreateNumber(node->n));
            break;
        case LDNodeString:
            cJSON_AddItemToArray(json, cJSON_CreateString(node->s));
            break;
        case LDNodeHash:
            cJSON_AddItemToArray(json, LDi_hashtojson(node->h));
            break;
        case LDNodeArray:
            cJSON_AddItemToArray(json, LDi_arraytojson(node->a));
            break;
        case LDNodeNone:
            break;
        }
    }

    return json;
}

/*
 * translation layer from json to hash. this does two things.
 * abstracts the cJSON interface from the user, allowing us to swap libraries.
 * also decodes and normalizes what I am calling "flavors" which are the
 * different payload variations sent by the LD servers which vary by endpoint.
 */

char *
LDi_hashtostring(const LDNode *const hash, const bool versioned)
{
    cJSON *json;
    if (versioned) {
        json = LDi_hashtoversionedjson(hash);
    } else {
        json = LDi_hashtojson(hash);
    }
    char *const tmp = cJSON_PrintUnformatted(json); LD_ASSERT(tmp);
    cJSON_Delete(json);
    char *const s = LDi_strdup(tmp); LD_ASSERT(s);
    free(tmp);
    return s;
}

LDNode *
jsontoarray(const cJSON *const json)
{
    LDNode *array = NULL;

    for (cJSON *item = json->child; item; item = item->next) {
        switch (item->type) {
        case cJSON_False:
            LDNodeAppendBool(&array, false);
            break;
        case cJSON_True:
            LDNodeAppendBool(&array, true);
            break;
        case cJSON_NULL:
            break;
        case cJSON_Number:
            LDNodeAppendNumber(&array, item->valuedouble);
            break;
        case cJSON_String:
            LDNodeAppendString(&array, item->valuestring);
            break;
        default:
            LDi_log(LD_LOG_FATAL, "jsontoarray unhandled case");
            abort();
            break;
        }
    }

    return array;
}

LDNode *
LDi_jsontohash(const cJSON *const json, const int flavor)
{
    LDNode *hash = NULL;

    for (const cJSON *item = json->child; item; item = item->next) {
        if (flavor == 2) {
            /* super gross, go back up a level */
            item = json;
        }
        const char *key = item->string;
        int version = 0;
        int variation = 0;
        double track = 0;
        int flagversion = 0;

        const cJSON *valueitem = item;
        switch (flavor) {
        case 0:
            /* plain json, no special handling */
            break;
        case 1:
            /* versioned, value is hiding one level down */
            /* grab the version first */
            for (valueitem = item->child; valueitem; valueitem = valueitem->next) {
                if (strcmp(valueitem->string, "version") == 0) {
                    version = (int)valueitem->valuedouble;
                }
                if (strcmp(valueitem->string, "flagVersion") == 0) {
                    flagversion = (int)valueitem->valuedouble;
                }
                if (strcmp(valueitem->string, "variation") == 0) {
                    variation = (int)valueitem->valuedouble;
                }
                if (strcmp(valueitem->string, "debugEventsUntilDate") == 0) {
                    track = valueitem->valuedouble;
                }
                if (strcmp(valueitem->string, "trackEvents") == 0) {
                    if (valueitem->type == cJSON_True)
                        track = 1;
                }
            }
            for (valueitem = item->child; valueitem; valueitem = valueitem->next) {
                if (strcmp(valueitem->string, "value") == 0) {
                    break;
                }
            }
            break;
        case 2:
            /* patch, the key is also hiding one level down */
            for (valueitem = item->child; valueitem; valueitem = valueitem->next) {
                if (strcmp(valueitem->string, "key") == 0) {
                    key = valueitem->valuestring;
                }
                if (strcmp(valueitem->string, "version") == 0) {
                    version = (int)valueitem->valuedouble;
                }
                if (strcmp(valueitem->string, "flagVersion") == 0) {
                    flagversion = (int)valueitem->valuedouble;
                }
                if (strcmp(valueitem->string, "variation") == 0) {
                    variation = (int)valueitem->valuedouble;
                }
                if (strcmp(valueitem->string, "debugEventsUntilDate") == 0) {
                    track = valueitem->valuedouble;
                }
                if (strcmp(valueitem->string, "trackEvents") == 0) {
                    if (valueitem->type == cJSON_True)
                        track = 1;
                }
            }
            for (valueitem = item->child; valueitem; valueitem = valueitem->next) {
                if (strcmp(valueitem->string, "value") == 0) {
                    break;
                }
            }
            break;
        }

        LDNode *node = NULL;

        if (valueitem) {
            switch (valueitem->type) {
            case cJSON_False:
                node = LDNodeAddBool(&hash, key, false);
                break;
            case cJSON_True:
                node = LDNodeAddBool(&hash, key, true);
                break;
            case cJSON_Number:
                node = LDNodeAddNumber(&hash, key, valueitem->valuedouble);
                break;
            case cJSON_String:
                node = LDNodeAddString(&hash, key, valueitem->valuestring);
                break;
            case cJSON_Array: {
                LDNode *array = jsontoarray(valueitem);
                node = LDNodeAddArray(&hash, key, array);
                break;
            }
            case cJSON_Object:
                node = LDNodeAddHash(&hash, key, LDi_jsontohash(valueitem, 0));
                break;
            default:
                break;
            }
        }

        if (!node) {
            node = LDNodeAddNone(&hash, key);
        }

        node->version = version;
        node->variation = variation;
        node->track = track;
        node->flagversion = flagversion;

        if (flavor == 2) {
            /* stop */
            break;
        }
    }
    return hash;
}

char *
LDNodeToJSON(const LDNode *const node)
{
    cJSON *json = NULL;
    switch (node->type) {
    case LDNodeArray:
        json = LDi_arraytojson(node);
        break;
    case LDNodeHash:
        json = LDi_hashtojson(node);
        break;
    default:
        LDi_log(LD_LOG_FATAL, "LDNodeToJSON not Array or Object");
        abort();
        break;
    }
    if (json) {
        char *const text = cJSON_PrintUnformatted(json); LD_ASSERT(text);
        cJSON_Delete(json);
        return text;
    }
    else {
        return NULL;
    }
}

LDNode *
LDNodeFromJSON(const char *const text)
{
    cJSON *const json = cJSON_Parse(text); LD_ASSERT(json);

    LDNode *output = NULL;

    switch (json->type) {
    case cJSON_Array:
        output = jsontoarray(json);
        break;
    case cJSON_Object:
        output = LDi_jsontohash(json, 0);
        break;
    default:
        LDi_log(LD_LOG_FATAL, "LDNodeFromJSON not Array or Object");
        abort();
        break;
    }

    cJSON_Delete(json);
    return output;
}

LDNode*
clonenode(const LDNode *const original)
{
    LDNode *const clone = LDAlloc(sizeof(*original));

    memcpy(clone, original, sizeof(*original));

    clone->key = NULL;

    switch (original->type) {
    case LDNodeString:
        clone->s = LDi_strdup(original->s);
        break;
    case LDNodeHash:
        clone->h = LDCloneHash(original->h);
        break;
    case LDNodeArray:
        clone->a = LDCloneArray(original->a);
        break;
    default:
        /* other cases have no dynamic memory */
        break;
    }

    return clone;
}

LDNode*
LDCloneHash(const LDNode *const original)
{
    LDNode *result = LDNodeCreateHash();

    for (const LDNode *node = original; node; node=node->hh.next) {
        LDNode *const clone = clonenode(node);
        clone->key = LDi_strdup(node->key);
        HASH_ADD_KEYPTR(hh, result, clone->key, strlen(clone->key), clone);
    }

    return result;
}

LDNode*
LDCloneArray(const LDNode *const original)
{
    LDNode *result = LDNodeCreateArray();

    for (const LDNode *node = original; node; node=node->hh.next) {
        LDNode *const clone = clonenode(node);
        clone->idx = node->idx;
        HASH_ADD_INT(result, idx, clone);
    }

    return result;
}

static void freehash(LDNode *node, bool);

static void
freenode(LDNode *const node, const bool freekey)
{
    if (freekey) {
        LDFree(node->key);
    }

    switch (node->type) {
    case LDNodeString:
        LDFree(node->s);
        break;
    case LDNodeHash:
        freehash(node->h, true);
        break;
    case LDNodeArray:
        freehash(node->a, false);
        break;
    default:
        /* other cases have no dynamic memory */
        break;
    }

    LDFree(node);
}

static void
freehash(LDNode *hash, const bool freekey)
{
    LDNode *node, *tmp;
    HASH_ITER(hh, hash, node, tmp) {
        HASH_DEL(hash, node);
        freenode(node, freekey);
    }
}

void
LDi_freenode(LDNode *const node)
{
    freenode(node, true);
}

void
LDi_freehash(LDNode *const hash)
{
    freehash(hash, true);
}
