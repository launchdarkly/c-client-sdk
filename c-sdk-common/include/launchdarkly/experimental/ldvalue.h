/*!
* @file ldvalue.h
* @brief LDValues represent immutable JSON values.
*/
#pragma once

#include <launchdarkly/boolean.h>
#include <launchdarkly/export.h>

/**
 * Describes the type of an LDValue.
 * These correspond to the standard types in JSON.
 */
enum LDValueType {
    /**
     * The value's type is unrecognized.
     */
    LDValueType_Unrecognized,
    /**
     * The value is null.
     */
    LDValueType_Null,
    /**
     * The value is a boolean.
     */
    LDValueType_Bool,
    /**
     * The value is a number. JSON does not have separate types for integers and floats.
     */
    LDValueType_Number,
    /**
     * The value is a string.
     */
    LDValueType_String,
    /**
     * The value is an array.
     */
    LDValueType_Array,
    /**
     * The value is an object.
     */
    LDValueType_Object
};

/**
 * LDValue is an opaque handle to immutable JSON data.
 *
 * Through various constructors and builders, the LDValue API can be used to build up
 * simple or complex JSON hierarchies. These can be passed into JSONVariation calls as default values,
 * or used in metrics.
 *
 * LDValue constructors can be used to create numbers, strings, booleans, and nulls.
 *
 * LDArray and LDObject are mutable builders, used to construct JSON arrays or objects.
 * The LDArray and LDObject builders are not themselves LDValues, but can be used to generate them.
 *
 * It is vital to understand the LDValue lifetime and ownership models in order to avoid double-frees and other
 * errors.
 *
 * LDValue lifetime model:
 * 1. LDValues live from creation with a primitive constructor (LDValue_Boolean and the like),
 *    or builder (LDObject_Build/LDArray_Build), until destruction with LDValue_Free.
 * 2. An LDValue's lifetime may be modified by adding it to an LDObject/LDArray. In this case
 *    there is no need to call LDValue_Free.
 * 3. LDValues returned by LDIter_Value live as long as the original LDValue from which the iterator was
 *    constructed.
 *
 * LDObject lifetime model:
 * 1. LDObjects live from creation with LDObject_New(), until destruction with LDObject_Free.
 * 2. An LDObject may be directly converted (moved) into an immutable LDValue with the LDValue_Object constructor.
 *    In this case, the lifetime of the LDObject is over and there is no need to call LDObject_Free.
 *
 * LDArray lifetime model:
 * 1. LDArrays lives from creation with LDArray_New(), until destruction with LDArray_Free,
 * 2. An LDArray may be directly converted (moved) into an immutable LDValue with the LDValue_Array constructor.
 *    In this case, the lifetime of the LDArray is over, and there is no need to call LDArray_Free.

 * LDIter lifetime model:
 * 1. LDIter lifetime begins with a call to LDValue_GetIter, and ends with that of the source LDValue.
 * 2. There is no need to free an LDIter.
 *
 * The ownership model of this API is unique ownership.
 *
 * LDValues created in the following ways are owned directly by the caller:
 * 1. Primitive constructors (LDValue_Boolean and the like),
 * 2. LDObject_Build or LDArray_Build
 * 3. Move construction from a builder via LDValue_Object or LDValue_Array
 *
 * LDValues created by LDIter_Value are owned by the source LDValue from which the iterator was created.
 *
 * LDValues moved into LDObjects or LDArrays are owned by that LDObject or LDArray.
 *
 * LDObjects and LDArrays are owned by the caller.
 *
 */
struct LDValue;
/**
 * LDObject is an opaque handle to a mutable object builder.
 */
struct LDObject;
/**
 * LDArray is an opaque handle to a mutable array builder.
 */
struct LDArray;
/**
 * LDIter is an opaque handle to an iterator, bound to an LDValue.
 * It can be used to obtain the keys and values of an LDObject, or the values of an LDArray.
 */
struct LDIter;

/**
 * Allocates a new boolean-type LDValue.
 * @param boolean LDBooleanTrue or LDBooleanFalse.
 * @return New LDValue.
 */
LD_EXPORT(struct LDValue *)
LDValue_Bool(LDBoolean boolean);

/**
 * Equivalent to LDValue_Bool(LDBooleanTrue).
 * @return New LDValue.
 */
LD_EXPORT(struct LDValue *)
LDValue_True(void);

/**
 * Equivalent to LDValue_Bool(LDBooleanFalse).
 * @return New LDValue.
 */
LD_EXPORT(struct LDValue *)
LDValue_False(void);

/**
 * Allocates a new null-type LDValue.
 * Note that a NULL pointer is not a valid LDValue; to represent null (the JSON type),
 * use this constructor.
 * @return New LDValue.
 */
LD_EXPORT(struct LDValue *)
LDValue_Null(void);

/**
 * Allocates a new number-type LDValue.
 * @param number Double value.
 * @return New LDValue.
 */
LD_EXPORT(struct LDValue *)
LDValue_Number(double number);

/**
 * Allocates a new non-owning string-type LDValue.
 *
 * Since the input is not owned by the LDValue, it will not be freed and MUST outlive the LaunchDarkly SDK.
 * See LDValue_OwnedString for a constructor that owns a copy of the input string.
 *
 * Note that this function still allocates in order to create the LDValue container itself.
 *
 * @param string Constant reference to a string. Cannot be NULL.
 * @return New LDValue.
 */
LD_EXPORT(struct LDValue *)
LDValue_ConstantString(const char *string);

/**
 * Allocates a new owning string-type LDValue.
 *
 * The input string will be copied. To avoid the copy, see LDValue_ConstantString.
 *
 * @param string Constant reference to a string. The string is copied. Cannot be NULL.
 * @return New LDValue.
 */
LD_EXPORT(struct LDValue *)
LDValue_OwnedString(const char *string);

/**
 * Creates an array-type LDValue from an LDArray.
 *
 * The input LDArray is consumed. Calling any functions on the LDArray may result in undefined behavior.
 *
 * To generate an LDValue without consuming the builder, see LDArray_Build.
 *
 * @param array LDArray to consume. Cannot be NULL.
 * @return New LDValue.
 */
LD_EXPORT(struct LDValue *)
LDValue_Array(struct LDArray *array);

/**
 * Creates an object-type LDValue from an LDObject.
 *
 * The input LDObject is consumed. Calling any functions on the LDObject may result in undefined behavior.
 *
 * To generate an LDValue without consuming the builder, see LDObject_Build.
 *
 * @param obj LDObject to consume. Cannot be NULL.
 * @return New LDValue.
 */
LD_EXPORT(struct LDValue *)
LDValue_Object(struct LDObject *obj);

/**
 * Allocates a deep clone of an existing LDValue.
 * @param source Source LDValue. Must not be `NULL`.
 * @return New LDValue.
 */
LD_EXPORT(struct LDValue *)
LDValue_Clone(struct LDValue *source);

/**
 * Returns the type of an LDValue.
 * @param value LDValue to inspect. Cannot be NULL.
 * @return Type of the LDValue, or LDValueType_Unrecognized if the type is unrecognized.
 */
LD_EXPORT(enum LDValueType)
LDValue_Type(struct LDValue *value);

/**
 * Performs a deep equality comparison between 'a' and 'b'.
 *
 * Warning: if the type of 'a' and 'b' are LDValueType_Object,
 * the comparison is O(N^2).
 *
 * @param a First LDValue to compare. Cannot be NULL.
 * @param b Second LDValue to compare. Cannot be NULL.
 * @return True if the LDValues are equal.
 */
LD_EXPORT(LDBoolean)
LDValue_Equal(struct LDValue *a, struct LDValue *b);

/**
 * Frees an LDValue.
 *
 * An LDValue should only be freed when directly owned by the caller, i.e.,
 * it was never moved into an LDArray or LDObject.
 *
 * @param value LDValue to free. No-op if NULL.
 */
LD_EXPORT(void)
LDValue_Free(struct LDValue *value);

/**
 * Parses a JSON string into a new LDValue.
 *
 * The string is not consumed.
 *
 * @param json Input JSON string. Cannot be NULL.
 * @return New LDValue if parsing succeeds, otherwise NULL.
 */
LD_EXPORT(struct LDValue *)
LDValue_ParseJSON(const char *json);

/**
 * Serializes the LDValue to a new, formatted JSON string.
 * @param value LDValue to serialize. Cannot be NULL.
 * @return Pointer to formatted JSON string. Free with LDFree.
 */
LD_EXPORT(char *)
LDValue_SerializeFormattedJSON(struct LDValue *value);

/**
 * Serializes the LDValue to a new, un-formatted JSON string.
 * @param value LDValue to serialize. Cannot be NULL.
 * @return Pointer to JSON string. Free with LDFree.
 */
LD_EXPORT(char *)
LDValue_SerializeJSON(struct LDValue *value);

/**
 * Obtain value of a number-type LDValue, otherwise return 0.
 * @param value Target LDValue. Cannot be NULL.
 * @return Number value, or 0 if not number-type.
 */
LD_EXPORT(double)
LDValue_GetNumber(struct LDValue *value);

/**
 * Obtain value of a boolean-type LDValue, otherwise returns LDBooleanFalse.
 *
 * @param value Target LDValue. Cannot be NULL.
 * @return Boolean value, or LDBooleanFalse if not boolean-type.
 */
LD_EXPORT(LDBoolean)
LDValue_GetBool(struct LDValue *value);
/**
 * Obtain value of a string-type LDValue, otherwise returns pointer
 * to an empty string.
 *
 * @param value Target LDValue. Cannot be NULL.
 * @return String value, or empty string if not string-type.
 */
LD_EXPORT(const char *)
LDValue_GetString(struct LDValue *value);

/**
 * Obtain iterator over an object or array-type LDValue, otherwise returns NULL.
 *
 * The iterator starts at the first element.
 *
 * @param value Target LDValue. Cannot be NULL.
 * @return Iterator, or NULL if not object-type or array-type. The iterator does not need
 * to be freed. Its lifetime is that of the LDValue.
 */
LD_EXPORT(struct LDIter *)
LDValue_GetIter(struct LDValue *value);
/**
 * Obtain number of LDValue elements stored in an array-type LDValue, or number
 * of key/LDValue pairs stored in an object-type LDValue.
 *
 * If not an array-type or object-type, returns 0.
 *
 * @param value Target LDValue. Cannot be NULL.
 * @return Count of LDValue elements, or 0 if not array-type/object-type.
 */
LD_EXPORT(unsigned int)
LDValue_Count(struct LDValue *value);

/**
 * Creates a new LDObject, which is a mutable builder for a key-value object.
 *
 * LDObjects may be turned into LDValues via LDObject_Build, or LDValue_Object,
 * depending on the use-case.
 *
 * @return New LDObject. Free with LDObject_Free, or transform into an LDValue
 * with LDValue_Object.
 */
LD_EXPORT(struct LDObject *)
LDObject_New(void);

/**
 * Recursively frees an LDObject and any children added with LDObject_Add(Constant|Owned)Key.
 * @param obj LDObject to free.
 */
LD_EXPORT(void)
LDObject_Free(struct LDObject *obj);

/**
 * Transfers ownership of an LDValue into an LDObject, named by the given key.
 * The key is a constant string reference and will not be freed.
 *
 * It is the responsibility of the caller to ensure key uniqueness. If called with
 * the same key multiple times, the JSON object will have duplicate keys, i.e:
 *
 * {"key1" : "foo", "key1" : "bar"}
 *
 * @param obj LDObject to mutate. Cannot be NULL.
 * @param key Constant string reference serving as a key. Cannot be NULL.
 * @param value LDValue to move; cannot be NULL. Since ownership is transferred, the caller
 * must not interact with the LDValue in any way after this call.
 */
LD_EXPORT(void)
LDObject_AddConstantKey(struct LDObject *obj, const char *key, struct LDValue *value);

/**
 * Transfers ownership of an LDValue into an LDObject, named by the given key.
 * The key will be cloned.
 *
 * It is the responsibility of the caller to ensure key uniqueness. If called with
 * the same key multiple times, the JSON object will have duplicate keys, i.e:
 *
 * {"key1" : "foo", "key1" : "bar"}
 *
 * @param obj LDObject to mutate. Cannot be NULL.
 * @param key Constant string reference serving as a key. Cannot be NULL. Will be cloned.
 * @param value LDValue to move; cannot be NULL. Since ownership is transferred, the caller
 * must not interact with the LDValue in any way after this call.
 */
LD_EXPORT(void)
LDObject_AddOwnedKey(struct LDObject *obj, const char *key, struct LDValue *value);

/**
 * Creates a new LDValue from an LDObject. The type of the LDValue is LDValueType_Object.
 * The LDObject is not freed; this function can be called multiple times.
 * @param obj Source LDObject. Cannot be NULL.
 * @return New LDValue. Free with LDValue_Free.
 */
LD_EXPORT(struct LDValue *)
LDObject_Build(struct LDObject *obj);

/**
 * Creates a new LDArray, which is a mutable builder for an array.
 *
 * LDArrays may be turned into LDValues via LDArray_Build, or LDValue_Array,
 * depending on the use-case.
 *
 * @return New LDArray. Free with LDArray_Free, or transform into LDValue with
 * LDValue_Array.
 */
LD_EXPORT(struct LDArray *)
LDArray_New(void);

/**
 * Recursively frees an LDArray and any elements added with LDArray_Add.
 * @param array LDArray to free.
 */
LD_EXPORT(void)
LDArray_Free(struct LDArray *array);

/**
 * Creates a new LDValue from an LDArray. The type of the LDValue is LDValueType_Array.
 * The LDArray is not freed; this function can be called multiple times.
 * @param array Source LDArray. Cannot be NULL.
 * @return New LDValue. Free with LDValue_Free.
 */
LD_EXPORT(struct LDValue *)
LDArray_Build(struct LDArray *array);

/**
 * Transfers ownership of an LDValue into an LDArray.
 * @param array LDArray to mutate. Cannot be NULL.
 * @param value LDValue to move; cannot be NULL. Treat the value as freed; it is unsafe to pass it to any
 * other function.
 */
LD_EXPORT(void)
LDArray_Add(struct LDArray *array, struct LDValue *value);

/**
 * Advance iterator to the next element, if present.
 * @param iterator Target LDIter. Cannot be NULL.
 * @return Iterator for next element.
 */
LD_EXPORT(struct LDIter *)
LDIter_Next(struct LDIter *iterator);

/**
 * Obtain key of current element, if source LDValue was object-type.
 *
 * @param iterator Target LDIter. Cannot be NULL. Iterator must be created
 * from object-type LDValue.
 * @return Key or NULL if not object-type.
 */
LD_EXPORT(const char *)
LDIter_Key(struct LDIter *iterator);

/**
 * Obtain non-owned reference to LDValue of current iterator element.
 * @param iterator Target LDIter. Cannot be NULL.
 * @return Non-owning reference to LDValue. The LDValue lives only as long as the source
 * LDValue from which the LDIter was originally created. It should not be freed.
 */
LD_EXPORT(struct LDValue *)
LDIter_Val(struct LDIter *iterator);

