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
newnode(LDNode **hash, const char *key, LDNodeType type)
{
    LDNode *node = LDAlloc(sizeof(*node));
    memset(node, 0, sizeof(*node));
    node->key = LDi_strdup(key);
    node->type = type;
    HASH_ADD_KEYPTR(hh, *hash, node->key, strlen(node->key), node);
    return node;
}

LDNode *
LDNodeAddNone(LDNode **hash, const char *key)
{
    LDNode *node = newnode(hash, key, LDNodeNone);
    return node;
}

LDNode *
LDNodeAddBool(LDNode **hash, const char *key, bool b)
{
    LDNode *node = newnode(hash, key, LDNodeBool);
    node->b = b;
    return node;
}

LDNode *
LDNodeAddNumber(LDNode **hash, const char *key, double n)
{
    LDNode *node = newnode(hash, key, LDNodeNumber);
    node->n = n;
    return node;
}

LDNode *
LDNodeAddString(LDNode **hash, const char *key, const char *s)
{
    LDNode *node = newnode(hash, key, LDNodeString);
    node->s = LDi_strdup(s);
    return node;
}

LDNode *
LDNodeAddHash(LDNode **hash, const char *key, LDNode *h)
{
    LDNode *node = newnode(hash, key, LDNodeHash);
    node->h = h;
    return node;
}

LDNode *
LDNodeAddArray(LDNode **hash, const char *key, LDNode *a)
{
    LDNode *node = newnode(hash, key, LDNodeArray);
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
appendnode(LDNode **array, LDNodeType type)
{
    LDNode *node = LDAlloc(sizeof(*node));
    memset(node, 0, sizeof(*node));
    node->idx = HASH_COUNT(*array);
    node->type = type;
    HASH_ADD_INT(*array, idx, node);
    return node;
}

LDNode *
LDNodeAppendBool(LDNode **array, bool b)
{
    LDNode *node = appendnode(array, LDNodeBool);
    node->b = b;
    return node;
}

LDNode *
LDNodeAppendNumber(LDNode **array, double n)
{
    LDNode *node = appendnode(array, LDNodeNumber);
    node->n = n;
    return node;
}

LDNode *
LDNodeAppendString(LDNode **array, const char *s)
{
    LDNode *node = appendnode(array, LDNodeString);
    node->s = LDi_strdup(s);
    return node;
}

LDNode *
LDNodeLookup(LDNode *hash, const char *key)
{
    LDNode *res = NULL;
    if (hash) {
        HASH_FIND_STR(hash, key, res);
    }
    return res;
}

LDNode *
LDNodeIndex(LDNode *array, unsigned int idx)
{
    LDNode *res = NULL;
    HASH_FIND_INT(array, &idx, res);
    return res;
}

unsigned int
LDNodeCount(LDNode *hash)
{
    return HASH_COUNT(hash);
}

void
LDNodeFree(LDNode **hash)
{
    LDi_freehash(*hash);
    *hash = NULL;
}

cJSON *
LDi_hashtoversionedjson(LDNode *hash)
{
    cJSON *json = cJSON_CreateObject();
    LDNode *node, *tmp;
    HASH_ITER(hh, hash, node, tmp) {
        cJSON *val = cJSON_CreateObject();
        switch (node->type) {
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
        if (node->version)
            cJSON_AddNumberToObject(val, "version", node->version);
        if (node->variation)
            cJSON_AddNumberToObject(val, "variation", node->variation);
        if (node->flagversion)
            cJSON_AddNumberToObject(val, "flagVersion", node->flagversion);

        cJSON_AddItemToObject(json, node->key, val);

    }
    return json;
}

cJSON *
LDi_hashtojson(LDNode *hash)
{
    cJSON *json = cJSON_CreateObject();
    LDNode *node, *tmp;
    HASH_ITER(hh, hash, node, tmp) {
        switch (node->type) {
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
LDi_arraytojson(LDNode *hash)
{
    cJSON *json = cJSON_CreateArray();
    LDNode *node, *tmp;
    HASH_ITER(hh, hash, node, tmp) {
        switch (node->type) {
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
LDi_hashtostring(LDNode *hash, bool versioned)
{
    cJSON *json;
    if (versioned) {
        json = LDi_hashtoversionedjson(hash);
    } else {
        json = LDi_hashtojson(hash);
    }
    char *tmp = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    char *s = LDi_strdup(tmp);
    free(tmp);
    return s;
}

LDNode *
jsontoarray(cJSON *json)
{
    LDNode *array = NULL;

    cJSON *item;
    for (item = json->child; item; item = item->next) {
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
            break;
        }
    }
    return array;
}

LDNode *
LDi_jsontohash(cJSON *json, int flavor)
{
    LDNode *hash = NULL;

    cJSON *item;
    for (item = json->child; item; item = item->next) {
        if (flavor == 2) {
            /* super gross, go back up a level */
            item = json;
        }
        const char *key = item->string;
        int version = 0;
        int variation = 0;
        double track = 0;
        int flagversion = 0;

        cJSON *valueitem = item;
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
LDNodeToJSON(LDNode *node)
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
        break;
    }
    if (json) {
        char *text = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
        return text;
    }
    else {
        return NULL;
    }
}

LDNode *
LDNodeFromJSON(const char *text)
{
    cJSON *json = cJSON_Parse(text);
    if (!json) {
        return NULL;
    }
    LDNode *output = NULL;
    switch (json->type) {
    case cJSON_Array:
        output = jsontoarray(json);
        break;
    case cJSON_Object:
        output = LDi_jsontohash(json, 0);
        break;
    }
    cJSON_Delete(json);
    return output;
}

static void freehash(LDNode *node, bool);

static void
freenode(LDNode *node, bool freekey)
{
    if (freekey)
        LDFree(node->key);
    if (node->type == LDNodeString)
        LDFree(node->s);
    if (node->type == LDNodeHash)
        freehash(node->h, true);
    if (node->type == LDNodeArray)
        freehash(node->a, false);
    LDFree(node);
}

static void
freehash(LDNode *hash, bool freekey)
{
    LDNode *node, *tmp;
    HASH_ITER(hh, hash, node, tmp) {
        HASH_DEL(hash, node);
        freenode(node, freekey);
    }
}

void
LDi_freenode(LDNode *node)
{
    freenode(node, true);
}

void
LDi_freehash(LDNode *hash)
{
    freehash(hash, true);
}
