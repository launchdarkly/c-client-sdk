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
    node->key = LDStrDup(key);
    node->type = type;
    node->variation = -1;
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
    node->s = LDStrDup(s);
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
    node->variation = -1;
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
    node->s = LDStrDup(s);
    return node;
}

LDNode *
LDNodeAppendArray(LDNode **const array, LDNode *const a)
{
    LDNode *const node = appendnode(array, LDNodeArray);
    node->a = a;
    return node;
}

LDNode *
LDNodeAppendHash(LDNode **const array, LDNode *const h)
{
    LDNode *const node = appendnode(array, LDNodeHash);
    node->h = h;
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

        if (node->reason) {
            cJSON_AddItemToObject(val, "reason", LDi_hashtojson(node->reason));
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
    char *const s = LDStrDup(tmp); LD_ASSERT(s);
    free(tmp);
    return s;
}

LDNode *
jsontoarray(const cJSON *const json)
{
    LDNode *array = NULL;

    for (const cJSON *item = json->child; item; item = item->next) {
        if (cJSON_IsFalse(item)){
            LDNodeAppendBool(&array, false);
        } else if (cJSON_IsTrue(item)) {
            LDNodeAppendBool(&array, true);
        } else if (cJSON_IsNull(item)) {
            /* ignore */
        } else if (cJSON_IsNumber(item)) {
            LDNodeAppendNumber(&array, item->valuedouble);
        } else if (cJSON_IsString(item)) {
            LDNodeAppendString(&array, item->valuestring);
        } else if (cJSON_IsArray(item)) {
            LDNode *const child = jsontoarray(item);
            LD_ASSERT(child);
            LDNodeAppendArray(&array, child);
        } else if (cJSON_IsObject(item)) {
            LDNode *const child = LDi_jsontohash(item, 0);
            LD_ASSERT(child);
            LDNodeAppendHash(&array, child);
        } else {
            LD_LOG(LD_LOG_FATAL, "jsontoarray unhandled case");
            abort();
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
        int variation = -1;
        double track = 0;
        int flagversion = 0;
        LDNode* reason = NULL;

        const cJSON *valueitem = item;

        // versioned, patch
        if (flavor == 1 || flavor == 2) {
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
                    if (cJSON_IsNumber(valueitem)) {
                        variation = (int)valueitem->valuedouble;
                    } else if (cJSON_IsNull(valueitem)) {
                        variation = -1;
                    } else {
                        LD_LOG(LD_LOG_ERROR, "variation not number or null");
                        variation = -1;
                    }
                }
                if (strcmp(valueitem->string, "debugEventsUntilDate") == 0) {
                    track = valueitem->valuedouble;
                }
                if (strcmp(valueitem->string, "trackEvents") == 0) {
                    if (valueitem->type == cJSON_True) {
                        track = 1;
                    }
                }
                if (strcmp(valueitem->string, "reason") == 0) {
                    LD_ASSERT(cJSON_IsObject(valueitem));
                    reason = LDi_jsontohash(valueitem, 0);
                }
            }
            for (valueitem = item->child; valueitem; valueitem = valueitem->next) {
                if (strcmp(valueitem->string, "value") == 0) {
                    break;
                }
            }
        }

        LDNode *node = NULL;

        if (valueitem) {
            if (cJSON_IsFalse(valueitem)) {
                node = LDNodeAddBool(&hash, key, false);
            } else if (cJSON_IsTrue(valueitem)) {
                node = LDNodeAddBool(&hash, key, true);
            } else if (cJSON_IsNumber(valueitem)) {
                node = LDNodeAddNumber(&hash, key, valueitem->valuedouble);
            } else if (cJSON_IsString(valueitem)) {
                node = LDNodeAddString(&hash, key, valueitem->valuestring);
            } else if (cJSON_IsArray(valueitem)) {
                LDNode *const array = jsontoarray(valueitem);
                node = LDNodeAddArray(&hash, key, array);
            } else if (cJSON_IsObject(valueitem)) {
                node = LDNodeAddHash(&hash, key, LDi_jsontohash(valueitem, 0));
            }
        }

        if (!node) {
            node = LDNodeAddNone(&hash, key);
        }

        node->version = version;
        node->variation = variation;
        node->track = track;
        node->flagversion = flagversion;
        node->reason = reason;

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
        LD_LOG(LD_LOG_FATAL, "LDNodeToJSON not Array or Object");
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

    if (cJSON_IsArray(json)) {
        output = jsontoarray(json);
    } else if (cJSON_IsObject(json)) {
        output = LDi_jsontohash(json, 0);
    } else {
        LD_LOG(LD_LOG_FATAL, "LDNodeFromJSON not Array or Object");
        abort();
    }

    cJSON_Delete(json);
    return output;
}

char *LDHashToJSON(const LDNode *const node)
{
    return LDi_hashtostring(node, false);
}

LDNode*
clonenode(const LDNode *const original)
{
    LDNode *const clone = LDAlloc(sizeof(*original));

    memcpy(clone, original, sizeof(*original));

    clone->key = NULL;

    switch (original->type) {
    case LDNodeString:
        clone->s = LDStrDup(original->s);
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
        clone->key = LDStrDup(node->key);
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

    LDi_freehash(node->reason);

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
