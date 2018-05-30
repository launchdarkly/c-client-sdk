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

LDMapNode *
LDMapLookup(LDMapNode *hash, const char *key)
{
    LDMapNode *res = NULL;
    LDMapNode *node, *tmp;

    HASH_FIND_STR(hash, key, res);
    return res;
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
        }
    }
    return json;
}

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
        case cJSON_Array:
            break;
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

void
LDi_freenode(LDMapNode *node)
{
    LDFree(node->key);
    if (node->type == LDNodeString)
        LDFree(node->s);
    if (node->type == LDNodeMap)
        LDi_freehash(node->m);
    LDFree(node);
}

void
LDi_freehash(LDMapNode *hash)
{
    LDMapNode *node, *tmp;
    HASH_ITER(hh, hash, node, tmp) {
        HASH_DEL(hash, node);
        LDi_freenode(node);
    }
}
