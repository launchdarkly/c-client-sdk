#include <stdbool.h>

#include "cJSON.h"

#include <launchdarkly/json.h>

#include "assertion.h"

struct LDJSON *
LDNewNull(void)
{
    return (struct LDJSON *)cJSON_CreateNull();
}

struct LDJSON *
LDNewBool(const LDBoolean boolean)
{
    return (struct LDJSON *)cJSON_CreateBool(boolean);
}

struct LDJSON *
LDNewNumber(const double number)
{
    return (struct LDJSON *)cJSON_CreateNumber(number);
}

struct LDJSON *
LDNewText(const char *const text)
{
    LD_ASSERT_API(text);

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (text == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDNewText NULL text");

            return NULL;
        }
    #endif

    return (struct LDJSON *)cJSON_CreateString(text);
}

struct LDJSON *
LDNewObject(void)
{
    return (struct LDJSON *)cJSON_CreateObject();
}

struct LDJSON *
LDNewArray(void)
{
    return (struct LDJSON *)cJSON_CreateArray();
}

LDBoolean
LDSetNumber(struct LDJSON *const rawNode, const double number)
{
    struct cJSON *const node = (struct cJSON *)rawNode;

    LD_ASSERT_API(node);
    LD_ASSERT_API(cJSON_IsNumber(node));

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (rawNode == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDSetNumber NULL node");

            return false;
        }

        if (cJSON_IsNumber(node) == false) {
            LD_LOG(LD_LOG_WARNING, "LDSetNumber node is not a number");

            return false;
        }
    #endif


    node->valuedouble = number;

    return true;
}

void
LDJSONFree(struct LDJSON *const json)
{
    cJSON_Delete((cJSON *)json);
}

struct LDJSON *
LDJSONDuplicate(const struct LDJSON *const input)
{
    LD_ASSERT_API(input);

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (input == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDJSONDuplicate NULL input");

            return NULL;
        }
    #endif

    return (struct LDJSON *)cJSON_Duplicate((cJSON *)input, true);
}

LDJSONType
LDJSONGetType(const struct LDJSON *const inputRaw)
{
    const struct cJSON *const input = (const struct cJSON *)inputRaw;

    LD_ASSERT_API(input);

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (input == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDJSONGetType NULL input");

            return LDNull;
        }
    #endif

    if (cJSON_IsBool(input)) {
        return LDBool;
    } else if (cJSON_IsNumber(input)) {
        return LDNumber;
    } else if (cJSON_IsNull(input)) {
        return LDNull;
    } else if (cJSON_IsObject(input)) {
        return LDObject;
    } else if (cJSON_IsArray(input)) {
        return LDArray;
    } else if (cJSON_IsString(input)) {
        return LDText;
    }

    LD_LOG(LD_LOG_CRITICAL, "LDJSONGetType unknown");

    #ifdef LAUNCHDARKLY_DEFENSIVE
        LD_LOG(LD_LOG_WARNING, "LDJSONGetType unknown type");

        return LDNull;
    #else
        LD_ASSERT(false);
    #endif
}

LDBoolean
LDJSONCompare(const struct LDJSON *const left, const struct LDJSON *const right)
{
    return cJSON_Compare((const cJSON *)left,
        (const cJSON *)right, true);
}

LDBoolean
LDGetBool(const struct LDJSON *const node)
{
    cJSON *const json = (cJSON *)node;

    LD_ASSERT_API(json);
    LD_ASSERT_API(cJSON_IsBool(json));

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (json == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDGetBool NULL node");

            return false;
        }

        if (cJSON_IsBool(json) == false) {
            LD_LOG(LD_LOG_WARNING, "LDGetBool not boolean");

            return false;
        }
    #endif

    return cJSON_IsTrue(json);
}

double
LDGetNumber(const struct LDJSON *const node)
{
    cJSON *const json = (cJSON *)node;

    LD_ASSERT_API(json);
    LD_ASSERT_API(cJSON_IsNumber(json));

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (json == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDGetNumber NULL node");

            return 0.0;
        }

        if (cJSON_IsNumber(json) == false) {
            LD_LOG(LD_LOG_WARNING, "LDGetNumber not number");

            return 0.0;
        }
    #endif

    return json->valuedouble;
}

const char *
LDGetText(const struct LDJSON *const node)
{
    cJSON *const json = (cJSON *)node;

    LD_ASSERT_API(json);
    LD_ASSERT_API(cJSON_IsString(json));

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (json == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDGetText NULL node");

            return NULL;
        }

        if (cJSON_IsString(json) == false) {
            LD_LOG(LD_LOG_WARNING, "LDGetText not text");

            return NULL;
        }
    #endif

    return json->valuestring;
}

struct LDJSON *
LDGetIter(const struct LDJSON *const rawCollection)
{
    const cJSON *const collection = (const cJSON *)rawCollection;

    LD_ASSERT_API(collection);
    LD_ASSERT_API(cJSON_IsArray(collection) || cJSON_IsObject(collection));

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (collection == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDGetIter NULL collection");

            return NULL;
        }

        if (!cJSON_IsArray(collection) && !cJSON_IsObject(collection)) {
            LD_LOG(LD_LOG_WARNING, "LDGetIter not Object or Array");

            return NULL;
        }
    #endif

    return (struct LDJSON *)collection->child;
}

struct LDJSON *
LDIterNext(const struct LDJSON *const rawIter)
{
    cJSON *const iter = (cJSON *)rawIter;

    LD_ASSERT_API(iter);

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (iter == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDIterNext NULL iter");

            return NULL;
        }
    #endif

    return (struct LDJSON *)iter->next;
}

const char *
LDIterKey(const struct LDJSON *const rawIter)
{
    cJSON *const iter = (cJSON *)rawIter;

    LD_ASSERT_API(iter);

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (iter == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDIterKey NULL iter");

            return NULL;
        }
    #endif

    return iter->string;
}

unsigned int
LDCollectionGetSize(const struct LDJSON *const rawCollection)
{
    cJSON *const collection = (cJSON *)rawCollection;

    LD_ASSERT_API(collection);
    LD_ASSERT_API(cJSON_IsArray(collection) || cJSON_IsObject(collection));

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (collection == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDCollectionGetSize NULL collection");

            return 0;
        }

        if (!cJSON_IsArray(collection) && !cJSON_IsObject(collection)) {
            LD_LOG(LD_LOG_WARNING, "LDCollectionGetSize not Object or Array");

            return 0;
        }
    #endif

    /* works for objects */
    return cJSON_GetArraySize(collection);
}

struct LDJSON *
LDCollectionDetachIter(struct LDJSON *const rawCollection,
    struct LDJSON *const rawIter)
{
    cJSON *const collection = (cJSON *)rawCollection;
    cJSON *const iter = (cJSON *)rawIter;

    LD_ASSERT_API(collection);
    LD_ASSERT_API(cJSON_IsArray(collection) || cJSON_IsObject(collection));
    LD_ASSERT_API(iter);

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (collection == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDCollectionDetachIter NULL collection");

            return NULL;
        }

        if (!cJSON_IsArray(collection) && !cJSON_IsObject(collection)) {
            LD_LOG(LD_LOG_WARNING,
                "LDCollectionDetachIter not Object or Array");

            return NULL;
        }

        if (iter == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDCollectionDetachIter NULL iter");

            return NULL;
        }
    #endif

    return (struct LDJSON *)cJSON_DetachItemViaPointer(collection, iter);
}

struct LDJSON *
LDArrayLookup(const struct LDJSON *const rawArray, const unsigned int index)
{
    cJSON *const array = (cJSON *)rawArray;

    LD_ASSERT_API(array);
    LD_ASSERT_API(cJSON_IsArray(array));

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (array == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDArrayLookup NULL array");

            return NULL;
        }

        if (cJSON_IsArray(array) == false) {
            LD_LOG(LD_LOG_WARNING, "LDArrayLookup not array");

            return NULL;
        }
    #endif

    return (struct LDJSON *)cJSON_GetArrayItem(array, index);
}

LDBoolean
LDArrayPush(struct LDJSON *const rawArray, struct LDJSON *const item)
{
    cJSON *const array = (cJSON *)rawArray;

    LD_ASSERT_API(array);
    LD_ASSERT_API(cJSON_IsArray(array));
    LD_ASSERT_API(item);

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (array == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDArrayPush NULL array");

            return false;
        }

        if (cJSON_IsArray(array) == false) {
            LD_LOG(LD_LOG_WARNING, "LDArrayPush not array");

            return false;
        }

        if (item == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDArrayPush NULL item");

            return false;
        }
    #endif

    cJSON_AddItemToArray(array, (cJSON *)item);

    return true;
}

LDBoolean
LDArrayAppend(struct LDJSON *const rawPrefix,
    const struct LDJSON *const rawSuffix)
{
    cJSON *iter;
    cJSON *const prefix = (cJSON *)rawPrefix;
    const cJSON *const suffix = (const cJSON *)rawSuffix;

    LD_ASSERT_API(prefix);
    LD_ASSERT_API(cJSON_IsArray(prefix));
    LD_ASSERT_API(suffix);
    LD_ASSERT_API(cJSON_IsArray(suffix));

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (prefix == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDArrayPush NULL prefix");

            return false;
        }

        if (cJSON_IsArray(prefix) == false) {
            LD_LOG(LD_LOG_WARNING, "LDArrayPush prefix not array");

            return false;
        }

        if (suffix == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDArrayPush NULL suffix");

            return false;
        }

        if (cJSON_IsArray(suffix) == false) {
            LD_LOG(LD_LOG_WARNING, "LDArrayPush suffix not array");

            return false;
        }
    #endif

    for (iter = suffix->child; iter; iter = iter->next) {
        cJSON *dupe;

        if (!(dupe = cJSON_Duplicate(iter, true))) {
            return false;
        }

        cJSON_AddItemToArray(prefix, dupe);
    }

    return true;
}

struct LDJSON *
LDObjectLookup(const struct LDJSON *const rawObject, const char *const key)
{
    cJSON *const object = (cJSON *)rawObject;

    LD_ASSERT_API(object);
    LD_ASSERT_API(cJSON_IsObject(object));
    LD_ASSERT_API(key);

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (object == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDObjectLookup NULL object");

            return NULL;
        }

        if (cJSON_IsObject(object) == false) {
            LD_LOG(LD_LOG_WARNING, "LDObjectLookup not object");

            return NULL;
        }

        if (key == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDObjectLookup NULL key");

            return NULL;
        }
    #endif

    return (struct LDJSON *)cJSON_GetObjectItemCaseSensitive(object, key);
}

LDBoolean
LDObjectSetKey(struct LDJSON *const rawObject,
    const char *const key, struct LDJSON *const item)
{
    cJSON *const object = (cJSON *)rawObject;

    LD_ASSERT_API(object);
    LD_ASSERT_API(cJSON_IsObject(object));
    LD_ASSERT_API(key);
    LD_ASSERT_API(item);

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (object == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDObjectSetKey NULL object");

            return false;
        }

        if (cJSON_IsObject(object) == false) {
            LD_LOG(LD_LOG_WARNING, "LDObjectSetKey not object");

            return false;
        }

        if (key == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDObjectSetKey NULL key");

            return false;
        }

        if (item == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDObjectSetKey NULL item");

            return false;
        }
    #endif

    cJSON_DeleteItemFromObjectCaseSensitive(object, key);

    cJSON_AddItemToObject(object, key, (cJSON *)item);

    return true;
}

void
LDObjectDeleteKey(struct LDJSON *const rawObject, const char *const key)
{
    cJSON *const object = (cJSON *)rawObject;

    LD_ASSERT_API(object);
    LD_ASSERT_API(cJSON_IsObject(object));
    LD_ASSERT_API(key);

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (object == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDObjectDeleteKey NULL object");

            return;
        }

        if (cJSON_IsObject(object) == false) {
            LD_LOG(LD_LOG_WARNING, "LDObjectDeleteKey not object");

            return;
        }

        if (key == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDObjectDeleteKey NULL key");

            return;
        }
    #endif

    cJSON_DeleteItemFromObjectCaseSensitive(object, key);
}

struct LDJSON *
LDObjectDetachKey(struct LDJSON *const rawObject, const char *const key)
{
    cJSON *const object = (cJSON *)rawObject;

    LD_ASSERT_API(object);
    LD_ASSERT_API(cJSON_IsObject(object));
    LD_ASSERT_API(key);

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (object == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDObjectDetachKey NULL object");

            return NULL;
        }

        if (cJSON_IsObject(object) == false) {
            LD_LOG(LD_LOG_WARNING, "LDObjectDetachKey not object");

            return NULL;
        }

        if (key == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDObjectDetachKey NULL key");

            return NULL;
        }
    #endif

    return (struct LDJSON *)
        cJSON_DetachItemFromObjectCaseSensitive(object, key);
}

LDBoolean
LDObjectMerge(struct LDJSON *const to, const struct LDJSON *const from)
{
    const struct LDJSON *iter;

    LD_ASSERT_API(to);
    LD_ASSERT_API(LDJSONGetType(to) == LDObject);
    LD_ASSERT_API(from);
    LD_ASSERT_API(LDJSONGetType(from) == LDObject);

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (to == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDObjectMerge NULL to");

            return false;
        }

        if (cJSON_IsObject((cJSON *)to) == false) {
            LD_LOG(LD_LOG_WARNING, "LDObjectMerge to not object");

            return false;
        }

        if (from == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDObjectMerge NULL from");

            return false;
        }

        if (cJSON_IsObject((const cJSON *)from) == false) {
            LD_LOG(LD_LOG_WARNING, "LDObjectMerge from not object");

            return false;
        }
    #endif

    for (iter = LDGetIter(from); iter; iter = LDIterNext(iter)) {
        struct LDJSON *duplicate;

        if (!(duplicate = LDJSONDuplicate(iter))) {
            return false;
        }

        LDObjectSetKey(to, LDIterKey(iter), duplicate);
    }

    return true;
}

char *
LDJSONSerialize(const struct LDJSON *const json)
{
    LD_ASSERT_API(json);

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (json == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDJSONSerialize NULL json");

            return NULL;
        }
    #endif

    return cJSON_PrintUnformatted((cJSON *)json);
}

struct LDJSON *
LDJSONDeserialize(const char *const text)
{
    LD_ASSERT_API(text);

    #ifdef LAUNCHDARKLY_DEFENSIVE
        if (text == NULL) {
            LD_LOG(LD_LOG_WARNING, "LDJSONDeserialize NULL text");

            return NULL;
        }
    #endif

    return (struct LDJSON *)cJSON_Parse(text);
}
