#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ldapi.h"
#include "ldinternal.h"


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
                printf("version with no value\n");
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
                printf("lost the value\n");
                continue;
            }
            break;
        }

        LDMapNode *node = malloc(sizeof(*node));
        memset(node, 0, sizeof(*node));
        node->key = strdup(key);

        switch (valueitem->type) {
        case cJSON_False:
            node->type = LDNodeBool;
            node->b = false;
            break;
        case cJSON_True:
            node->type = LDNodeBool;
            node->b = true;
            break;
        case cJSON_NULL:
            break;
        case cJSON_Number:
            node->type = LDNodeNumber;
            node->n = valueitem->valuedouble;
            break;
        case cJSON_String:
            node->type = LDNodeString;
            node->s = strdup(valueitem->valuestring);
            break;
        case cJSON_Array:
            break;
        case cJSON_Object:
            node->type = LDNodeMap;
            printf("going down... %s\n", valueitem->string);
            node->m = LDi_jsontohash(valueitem, 0);
            break;
        default:
            break;
        }
        HASH_ADD_KEYPTR(hh, hash, node->key, strlen(node->key), node);
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
    free(node->key);
    if (node->type == LDNodeString)
        free(node->s);
    if (node->type == LDNodeMap)
        LDi_freehash(node->m);
    free(node);
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
