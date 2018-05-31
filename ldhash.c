#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ldapi.h"
#include "ldinternal.h"

LDMapNode *
LDMapCreate()
{
    /* empty hash is represented by NULL pointer */
    return NULL;
}

static LDMapNode *
newnode(LDMapNode **hash, const char *key, LDNodeType type)
{
    LDMapNode *node = LDAlloc(sizeof(*node));
    memset(node, 0, sizeof(*node));
    node->key = LDi_strdup(key);
    node->type = type;
    HASH_ADD_KEYPTR(hh, *hash, node->key, strlen(node->key), node);
    return node;
}

void
LDMapAddBool(LDMapNode **hash, const char *key, bool b)
{
    LDMapNode *node = newnode(hash, key, LDNodeBool);
    node->b = b;
}

void
LDMapAddNumber(LDMapNode **hash, const char *key, double n)
{
    LDMapNode *node = newnode(hash, key, LDNodeNumber);
    node->n = n;
}

void
LDMapAddString(LDMapNode **hash, const char *key, const char *s)
{
    LDMapNode *node = newnode(hash, key, LDNodeString);
    node->s = LDi_strdup(s);
}

void
LDMapAddMap(LDMapNode **hash, const char *key, LDMapNode *m)
{
    LDMapNode *node = newnode(hash, key, LDNodeMap);
    node->m = m;
}

void
LDMapAddArray(LDMapNode **hash, const char *key, LDMapNode *a)
{
    LDMapNode *node = newnode(hash, key, LDNodeArray);
    node->a = a;
}

LDMapNode *
LDMapArray()
{
    /* same thing as map actually */
    return NULL;
}

static LDMapNode *
appendnode(LDMapNode **array, LDNodeType type)
{
    LDMapNode *node = LDAlloc(sizeof(*node));
    memset(node, 0, sizeof(*node));
    node->idx = HASH_COUNT(*array);
    node->type = type;
    HASH_ADD_INT(*array, idx, node);
    return node;
}

void
LDMapAppendBool(LDMapNode **array, bool b)
{
    LDMapNode *node = appendnode(array, LDNodeBool);
    node->b = b;
}

void
LDMapAppendNumber(LDMapNode **array, double n)
{
    LDMapNode *node = appendnode(array, LDNodeNumber);
    node->n = n;
}

void
LDMapAppendString(LDMapNode **array, const char *s)
{
    LDMapNode *node = appendnode(array, LDNodeString);
    node->s = LDi_strdup(s);
}

LDMapNode *
LDMapLookup(LDMapNode *hash, const char *key)
{
    LDMapNode *res = NULL;
    HASH_FIND_STR(hash, key, res);
    return res;
}

LDMapNode *
LDMapIndex(LDMapNode *array, unsigned int idx)
{
    LDMapNode *res = NULL;
    HASH_FIND_INT(array, &idx, res);
    return res;
}

unsigned int
LDMapCount(LDMapNode *hash)
{
    return HASH_COUNT(hash);
}

void
LDMapFree(LDMapNode **hash)
{
    LDi_freehash(*hash);
    *hash = NULL;
}

cJSON *
LDi_hashtojson(LDMapNode *hash)
{
    cJSON *json = cJSON_CreateObject();
    LDMapNode *node, *tmp;
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
        case LDNodeMap:
            cJSON_AddItemToObject(json, node->key, LDi_hashtojson(node->m));
            break;
        case LDNodeArray:
            cJSON_AddItemToObject(json, node->key, LDi_arraytojson(node->a));
            break;
        }
    }
    return json;
}

cJSON *
LDi_arraytojson(LDMapNode *hash)
{
    cJSON *json = cJSON_CreateArray();
    LDMapNode *node, *tmp;
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
        case LDNodeMap:
            cJSON_AddItemToArray(json, LDi_hashtojson(node->m));
            break;
        case LDNodeArray:
            cJSON_AddItemToArray(json, LDi_arraytojson(node->a));
            break;
        }
    }
    return json;
}

/*
 * translation layer from json to map. this does two things.
 * abstracts the cJSON interface from the user, allowing us to swap libraries.
 * also decodes and normalizes what I am calling "flavors" which are the 
 * different payload variations sent by the LD servers which vary by endpoint.
 */

char *
LDi_hashtostring(LDMapNode *hash)
{
    cJSON *json = LDi_hashtojson(hash);
    char *tmp = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    char *s = LDi_strdup(tmp);
    free(tmp);
    return s;
}

LDMapNode *
jsontoarray(cJSON *json)
{
    LDMapNode *array = NULL;

    cJSON *item;
    for (item = json->child; item; item = item->next) {
        switch (item->type) {
        case cJSON_False:
            LDMapAppendBool(&array, false);
            break;
        case cJSON_True:
            LDMapAppendBool(&array, true);
            break;
        case cJSON_NULL:
            break;
        case cJSON_Number:
            LDMapAppendNumber(&array, item->valuedouble);
            break;
        case cJSON_String:
            LDMapAppendString(&array, item->valuestring);
            break;
        default:
            break;
        }
    }
    return array;
}

LDMapNode *
LDi_jsontohash(cJSON *json, int flavor)
{
    LDMapNode *hash = NULL;

    cJSON *item;
    for (item = json->child; item; item = item->next) {
        if (flavor == 2) {
            /* super gross, go back up a level */
            item = json;
        }
        const char *key = item->string;
        
        cJSON *valueitem = item;
        switch (flavor) {
        case 0:
            /* plain json, no special handling */
            break;
        case 1:
            /* versioned, value is hiding one level down */
            for (valueitem = item->child; valueitem; valueitem = valueitem->next) {
                if (strcmp(valueitem->string, "value") == 0) {
                    break;
                }
            }
            if (!valueitem) {
                LDi_log(5, "version with no value\n");
                continue;
            }
            break;
        case 2:
            /* patch, the key is also hiding one level down */
            for (valueitem = item->child; valueitem; valueitem = valueitem->next) {
                if (strcmp(valueitem->string, "key") == 0) {
                    key = valueitem->valuestring;
                    break;
                }
            }
            for (valueitem = item->child; valueitem; valueitem = valueitem->next) {
                if (strcmp(valueitem->string, "value") == 0) {
                    break;
                }
            }
            if (!valueitem) {
                LDi_log(5, "lost the value\n");
                continue;
            }
            break;
        }

        switch (valueitem->type) {
        case cJSON_False:
            LDMapAddBool(&hash, key, false);
            break;
        case cJSON_True:
            LDMapAddBool(&hash, key, true);
            break;
        case cJSON_NULL:
            break;
        case cJSON_Number:
            LDMapAddNumber(&hash, key, valueitem->valuedouble);
            break;
        case cJSON_String:
            LDMapAddString(&hash, key, valueitem->valuestring);
            break;
        case cJSON_Array: {
            LDMapNode *array = jsontoarray(valueitem);
            LDMapAddArray(&hash, key, array);
            break;
        }
        case cJSON_Object:
            LDMapAddMap(&hash, key, LDi_jsontohash(valueitem, 0));
            break;
        default:
            break;
        }
        
        if (flavor == 2) {
            /* stop */
            break;
        }
    }
    return hash;
}

static void freehash(LDMapNode *node, bool);

static void
freenode(LDMapNode *node, bool freekey)
{
    if (freekey)
        LDFree(node->key);
    if (node->type == LDNodeString)
        LDFree(node->s);
    if (node->type == LDNodeMap)
        freehash(node->m, true);
    if (node->type == LDNodeArray)
        freehash(node->a, false);
    LDFree(node);
}

static void
freehash(LDMapNode *hash, bool freekey)
{
    LDMapNode *node, *tmp;
    HASH_ITER(hh, hash, node, tmp) {
        HASH_DEL(hash, node);
        freenode(node, freekey);
    }
}

void
LDi_freenode(LDMapNode *node)
{
    freenode(node, true);
}

void
LDi_freehash(LDMapNode *hash)
{
    freehash(hash, true);
}
