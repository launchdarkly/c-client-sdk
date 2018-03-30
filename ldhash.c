#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ldapi.h"
#include "ldinternal.h"


LDMapNode *
LDi_jsontohash(cJSON *json)
{
    LDMapNode *hash = NULL;

    cJSON *item = json->child;
    while (item != NULL) {
        LDMapNode *node = malloc(sizeof(*node));
        memset(node, 0, sizeof(*node));
        node->key = strdup(item->string);
        switch (item->type) {
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
            node->n = item->valuedouble;
            break;
        case cJSON_String:
            node->type = LDNodeString;
            node->s = strdup(item->valuestring);
            break;
        case cJSON_Array:
            break;
        case cJSON_Object:
            break;
        default:
            break;
        }
        HASH_ADD_KEYPTR(hh, hash, node->key, strlen(node->key), node);
        item = item->next;
    }
    return hash;
}

void
LDi_freehash(LDMapNode *hash)
{
    LDMapNode *node, *tmp;
    HASH_ITER(hh, hash, node, tmp) {
        HASH_DEL(hash, node);
        free(node->key);
        if (node->type == LDNodeString)
            free(node->s);
        free(node);
    }

}