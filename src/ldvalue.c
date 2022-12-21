/*
 * LDValue is implemented as a thin wrapper around the cJSON API.
 * It aims to provide a clear ownership/lifetime model via explicit
 * types for the immutable LDValue, object and array builders, and iterators.
 *
 * Because it is implemented in terms of cJSON, it is compatible with the
 * existing LDJSON API.
 *
 */

#include <launchdarkly/experimental/ldvalue.h>
#include "assertion.h"
#include "cJSON.h"

#define AS_CJSON(ptr) ((struct cJSON*)(ptr))

#define AS_LDVALUE(ptr) ((struct LDValue*)(ptr))

#define AS_LDOBJECT(ptr) ((struct LDObject*)(ptr))

#define AS_LDARRAY(ptr) ((struct LDArray*)(ptr))

#define AS_LDITER(ptr) ((struct LDIter*)(ptr))

#define LD_BOOL_DEFAULT LDBooleanFalse

#define LD_NUMBER_DEFAULT 0.0

#define LD_STRING_DEFAULT ""

#define LD_ITER_DEFAULT NULL

struct LDValue* LDValue_Null(void) {
    return AS_LDVALUE(cJSON_CreateNull());
}

struct LDValue* LDValue_Bool(LDBoolean boolVal) {
    return AS_LDVALUE(cJSON_CreateBool(boolVal));
}

struct LDValue *LDValue_True(void) {
    return LDValue_Bool(LDBooleanTrue);
}

struct LDValue *LDValue_False(void) {
    return LDValue_Bool(LDBooleanFalse);
}

struct LDValue *LDValue_Number(double number) {
    return AS_LDVALUE(cJSON_CreateNumber(number));
}

struct LDValue *LDValue_ConstantString(const char *string) {
    LD_ASSERT_API(string);
    return AS_LDVALUE(cJSON_CreateStringReference(string));
}

struct LDValue *LDValue_OwnedString(const char *string) {
    LD_ASSERT_API(string);
    return AS_LDVALUE(cJSON_CreateString(string));
}

enum LDValueType LDValue_Type(struct LDValue *value) {
    struct cJSON *v = AS_CJSON(value);
    if (cJSON_IsBool(v)) {
        return LDValueType_Bool;
    } else if (cJSON_IsNumber(v)) {
        return LDValueType_Number;
    } else if (cJSON_IsNull(v)) {
        return LDValueType_Null;
    } else if (cJSON_IsObject(v)) {
        return LDValueType_Object;
    } else if (cJSON_IsArray(v)) {
        return LDValueType_Array;
    } else if (cJSON_IsString(v)) {
        return LDValueType_String;
    }
    return LDValueType_Unrecognized;
}


struct LDValue *LDValue_Array(struct LDArray *array) {
    LD_ASSERT_API(array);
    return AS_LDVALUE(array);
}

struct LDValue *LDValue_Object(struct LDObject *obj) {
    LD_ASSERT_API(obj);
    return AS_LDVALUE(obj);
}

struct LDValue *LDValue_Clone(struct LDValue *source) {
    LD_ASSERT_API(source);
    return AS_LDVALUE(cJSON_Duplicate(AS_CJSON(source), LDBooleanTrue));
}

char *
LDValue_SerializeFormattedJSON(struct LDValue *value) {
    LD_ASSERT_API(value);
    return cJSON_Print(AS_CJSON(value));
}

char *
LDValue_SerializeJSON(struct LDValue *value) {
    LD_ASSERT_API(value);
    return cJSON_PrintUnformatted(AS_CJSON(value));
}

LDBoolean LDValue_Equal(struct LDValue *left, struct LDValue *right) {
    LD_ASSERT_API(left);
    LD_ASSERT_API(right);
    return (LDBoolean)
        cJSON_Compare(AS_CJSON(left), AS_CJSON(right), LDBooleanTrue);
}

void LDValue_Free(struct LDValue *value) {
    cJSON_Delete(AS_CJSON(value));
}

struct LDValue*
LDValue_ParseJSON(const char *json) {
    LD_ASSERT_API(json);
    return AS_LDVALUE(cJSON_Parse(json));
}

double
LDValue_GetNumber(struct LDValue *value) {
    LD_ASSERT_API(value);
    if (!cJSON_IsNumber(AS_CJSON(value))) {
        return LD_NUMBER_DEFAULT;
    }
    return AS_CJSON(value)->valuedouble;
}

LDBoolean
LDValue_GetBool(struct LDValue *value) {
    LD_ASSERT_API(value);
    if (!cJSON_IsBool(AS_CJSON(value))) {
        return LD_BOOL_DEFAULT;
    }
    return (LDBoolean)cJSON_IsTrue(AS_CJSON(value));
}

const char*
LDValue_GetString(struct LDValue *value) {
    LD_ASSERT_API(value);
    if (!cJSON_IsString(AS_CJSON(value))) {
        return LD_STRING_DEFAULT;
    }
    return AS_CJSON(value)->valuestring;
}

struct LDIter *
LDValue_GetIter(struct LDValue *value) {
    LD_ASSERT_API(value);
    if (!(cJSON_IsObject(AS_CJSON(value)) || cJSON_IsArray(AS_CJSON(value)))) {
        return LD_ITER_DEFAULT;
    }
    return AS_LDITER(AS_CJSON(value)->child);
}

unsigned int LDValue_Count(struct LDValue *value) {
    if (!(cJSON_IsObject(AS_CJSON(value)) || cJSON_IsArray(AS_CJSON(value)))) {
        return 0;
    }
    return cJSON_GetArraySize(AS_CJSON(value));
}

struct LDObject *LDObject_New() {
    return AS_LDOBJECT(cJSON_CreateObject());
}

void LDObject_Free(struct LDObject *obj) {
    cJSON_Delete(AS_CJSON(obj));
}

void LDObject_AddConstantKey(struct LDObject *obj, const char *key, struct LDValue *value) {
    LD_ASSERT_API(obj);
    LD_ASSERT_API(key);
    LD_ASSERT_API(value);
    cJSON_AddItemToObjectCS(AS_CJSON(obj), key, AS_CJSON(value));
}

void LDObject_AddOwnedKey(struct LDObject *obj, const char *key, struct LDValue *value) {
    LD_ASSERT_API(obj);
    LD_ASSERT_API(key);
    LD_ASSERT_API(value);
    cJSON_AddItemToObject(AS_CJSON(obj), key, AS_CJSON(value));
}


struct LDValue *LDObject_Build(struct LDObject *obj) {
    LD_ASSERT_API(obj);
    return AS_LDVALUE(cJSON_Duplicate(AS_CJSON(obj), LDBooleanTrue));
}

struct LDArray* LDArray_New(void) {
    return AS_LDARRAY(cJSON_CreateArray());
}

void LDArray_Free(struct LDArray *array) {
    cJSON_Delete(AS_CJSON(array));
}

void LDArray_Add(struct LDArray* array, struct LDValue *child) {
    LD_ASSERT_API(array);
    LD_ASSERT_API(child);
    cJSON_AddItemToArray(AS_CJSON(array), AS_CJSON(child));
}

struct LDValue *LDArray_Build(struct LDArray *array) {
    LD_ASSERT_API(array);
    return AS_LDVALUE(cJSON_Duplicate(AS_CJSON(array), LDBooleanTrue));
}


struct LDIter* LDIter_Next(struct LDIter *iterator) {
    LD_ASSERT_API(iterator);
    return AS_LDITER(AS_CJSON(iterator)->next);
}

const char *LDIter_Key(struct LDIter *iterator) {
    LD_ASSERT_API(iterator);
    return AS_CJSON(iterator)->string;
}

struct LDValue *LDIter_Val(struct LDIter *iterator) {
    LD_ASSERT_API(iterator);
    return AS_LDVALUE(iterator);
}
