/*!
 * @file json.h
 * @brief Public API Interface for JSON usage
 */

#pragma once

#include <launchdarkly/export.h>
#include <launchdarkly/boolean.h>

/* **** Forward Declarations **** */

struct LDJSON;

/** @brief Represents the type of a LaunchDarkly JSON node */
typedef enum {
    /** @brief JSON Null (not JSON undefined) */
    LDNull = 0,
    /** @brief UTF-8 JSON string */
    LDText,
    /** @brief JSON number (Double or Integer) */
    LDNumber,
    /** @brief JSON boolean (True or False) */
    LDBool,
    /** @brief JSON string indexed map */
    LDObject,
    /** @brief JSON integer indexed array */
    LDArray
} LDJSONType;

/***************************************************************************//**
 * @name Constructing values
 * Routines for constructing values
 * @{
 ******************************************************************************/

/**
 * @brief Constructs a JSON node of type `LDJSONNull`.
 * @return `NULL` on failure.
 */
LD_EXPORT(struct LDJSON *) LDNewNull(void);

/**
 * @brief Constructs a JSON node of type `LDJSONBool`.
 * @param[in] boolean The value to assign the new node
 * @return `NULL` on failure.
 */
LD_EXPORT(struct LDJSON *) LDNewBool(const LDBoolean boolean);

/**
 * @brief Constructs a JSON node of type `LDJSONumber`.
 * @param[in] number The value to assign the new node
 * @return `NULL` on failure.
 */
LD_EXPORT(struct LDJSON *) LDNewNumber(const double number);

/**
 * @brief Returns a new constructed JSON node of type `LDJSONText`.
 * @param[in] text The text to copy and then assign the new node.
 * May not be `NULL`.
 * @return `NULL` on failure.
 */
LD_EXPORT(struct LDJSON *) LDNewText(const char *const text);

/**
 * @brief Constructs a JSON node of type `LDJSONObject`.
 * @return `NULL` on failure.
 */
LD_EXPORT(struct LDJSON *) LDNewObject(void);

/**
 * @brief Constructs a JSON node of type `LDJSONArray`.
 * @return `NULL` on failure.
 */
LD_EXPORT(struct LDJSON *) LDNewArray(void);

/*@}*/

/***************************************************************************//**
 * @name Setting values
 * Routines for setting values of existing nodes
 * @{
 ******************************************************************************/

 /**
  * @brief Set the value of an existing Number.
  * @param[in] node The node to set the value of. Must be a number.
  * @param[in] number The value to use for the node.
  * @return True on success, False on failure.
  */
LD_EXPORT(LDBoolean) LDSetNumber(struct LDJSON *const node,
    const double number);

/*@}*/

/***************************************************************************//**
 * @name Cleanup and Utility
 * Miscellaneous Operations
 * @{
 ******************************************************************************/

/**
 * @brief Frees any allocated JSON structure.
 * @param[in] json May be `NULL`.
 * @return Void.
 */
LD_EXPORT(void) LDJSONFree(struct LDJSON *const json);

/**
 * @brief Duplicates an existing JSON strucutre. This acts as a deep copy.
 * @param[in] json JSON to be duplicated. May not be `NULL`.
 * @return `NULL` on failure
 */
LD_EXPORT(struct LDJSON *) LDJSONDuplicate(const struct LDJSON *const json);

/**
 * @brief Get the type of a JSON structure
 * @param[in] json May be not be `NULL`.
 * @return The JSON type, or `LDNull` on failure.
 */
LD_EXPORT(LDJSONType) LDJSONGetType(const struct LDJSON *const json);

/**
 * @brief Deep compare to check if two JSON structures are equal
 * @param[in] left May be `NULL`.
 * @param[in] right May be `NULL`.
 * @return True if equal, false otherwise.
 */
LD_EXPORT(LDBoolean) LDJSONCompare(const struct LDJSON *const left,
    const struct LDJSON *const right);

/*@}*/

/***************************************************************************//**
 * @name Reading Values
 * Routines for reading values
 * @{
 ******************************************************************************/

/**
 * @brief Get the value from a node of type `LDJSONBool`.
 * @param[in] node Node to read value from. Must be correct type.
 * @return The boolean nodes value. Returns false on failure.
 */
LD_EXPORT(LDBoolean) LDGetBool(const struct LDJSON *const node);

/**
 * @brief Get the value from a node of type `LDJSONNumber`.
 * @param[in] node Node to read value from. Must be correct type.
 * @return The number nodes value. Returns 0 on failure.
 */
LD_EXPORT(double) LDGetNumber(const struct LDJSON *const node);

/**
 * @brief Get the value from a node of type `LDJSONText`.
 * @param[in] node Node to read value from. Must be correct type.
 * @return The text nodes value. `NULL` on failure.
 */
LD_EXPORT(const char *) LDGetText(const struct LDJSON *const node);

/*@}*/

/***************************************************************************//**
 * @name Iterator Operations
 * Routines for working with collections (Objects, Arrays)
 * @{
 ******************************************************************************/

/**
 * @brief Returns the next item in the sequence
 * @param[in] iter May be not be NULL (assert)
 * @return Item, or `NULL` if the iterator is finished.
 */
LD_EXPORT(struct LDJSON *) LDIterNext(const struct LDJSON *const iter);

/**
 * @brief Allows iteration over an array. Modification of the array invalidates
 * this iterator.
 * @param[in] collection May not be `NULL`.
 * must be of type `LDJSONArray` or `LDJSONObject`.
 * @return First child iterator, or `NULL` if empty or on failure.
 */
LD_EXPORT(struct LDJSON *) LDGetIter(const struct LDJSON *const collection);

/**
 * @brief Returns the key associated with the iterator
 * Must be an object iterator.
 * @param[in] iter The iterator obtained from an object.
 * May not be `NULL`.
 * @return The key on success, or `NULL` if there is no key, or on failure.
 */
LD_EXPORT(const char *) LDIterKey(const struct LDJSON *const iter);

/**
 * @brief Return the size of a JSON collection
 * @param[in] collection May not be `NULL`.
 * must be of type `LDJSONArray` or `LDJSONObject`.
 * @return The size of the collection, or zero on failure.
 */
LD_EXPORT(unsigned int) LDCollectionGetSize(
    const struct LDJSON *const collection);

/**
 * @brief Remove an iterator from a collection
 * @param[in] collection May not be `NULL`.
 * must be of type `LDJSONArray` or `LDJSONObject`.
 * @param[in] iter May not be `NULL`.
 * @return The detached iterator, or `NULL` on failure.
 */
LD_EXPORT(struct LDJSON *) LDCollectionDetachIter(
    struct LDJSON *const collection, struct LDJSON *const iter);

/*@}*/

/***************************************************************************//**
 * @name Array Operations
 * Routines for working with arrays
 * @{
 ******************************************************************************/

/**
 * @brief Lookup up the value of an index for a given array
 * @param[in] array May not be `NULL`.
 * must be of type `LDJSONArray`.
 * @param[in] index The index to lookup in the array
 * @return Item if it exists, otherwise `NULL` if does not exist, or on failure.
 */
LD_EXPORT(struct LDJSON *) LDArrayLookup(const struct LDJSON *const array,
    const unsigned int index);

/**
 * @brief Adds an item to the end of an existing array.
 * @param[in] array Must be of type `LDJSONArray`.
 * @param[in] item The value to append to the array. This item is consumed.
 * @return True on success, False on failure.
 */
LD_EXPORT(LDBoolean) LDArrayPush(struct LDJSON *const array,
    struct LDJSON *const item);

/**
 * @brief Appends the array on the right to the array on the left
 * @param[in] prefix Must be of type `LDJSONArray`.
 * @param[in] suffix Must be of type `LDJSONArray`.
 * @return True on success, False on failure.
 */
LD_EXPORT(LDBoolean) LDArrayAppend(struct LDJSON *const prefix,
    const struct LDJSON *const suffix);

/***************************************************************************//**
 * @name Object Operations
 * Routines for working with objects
 * @{
 ******************************************************************************/

/**
 * @brief Lookup up the value of a key for a given object
 * @param[in] object May not be `NULL`.
 * must be of type `LDJSONObject`.
 * @param[in] key The key to lookup in the object. May not be `NULL`.
 * @return The item if it exists, otherwise `NULL`.
 */
LD_EXPORT(struct LDJSON *) LDObjectLookup(const struct LDJSON *const object,
    const char *const key);

/**
 * @brief Sets the provided key in an object to item.
 * If the key already exists the original value is deleted.
 * @param[in] object Must be of type `LDJSONObject`.
 * @param[in] key The key that is being written to in the object.
 * May not be `NULL`.
 * @param[in] item The value to assign to key. This item is consumed.
 * @return True on success, False on failure.
 */
LD_EXPORT(LDBoolean) LDObjectSetKey(struct LDJSON *const object,
    const char *const key, struct LDJSON *const item);

/**
 * @brief Delete the provided key from the given object.
 * @param[in] object May not be `NULL`.
 * must be of type `LDJSONObject`.
 * @param[in] key The key to delete from the object. May not be `NULL`.
 * @return Void
 */
LD_EXPORT(void) LDObjectDeleteKey(struct LDJSON *const object,
    const char *const key);

/**
 * @brief Detach the provided key from the given object. The returned value is
 * no longer owned by the object and must be manually deleted.
 * @param[in] object May not be `NULL`.
 * must be of type `LDJSONObject`.
 * @param[in] key The key to detach from the object. May not be `NULL`.
 * @return The value associated, or `NULL` if it does not exit, or on error.
 */
LD_EXPORT(struct LDJSON *) LDObjectDetachKey(struct LDJSON *const object,
    const char *const key);

/**
 * @brief Copy keys from one object to another. If a key already exists it is
 * overwritten by the new value.
 * @param[in] to Object to assign to. May not be `NULL`.
 * @param[in] from Object to copy keys from. May not be `NULL`.
 * @return True on success, `to` is polluted on failure.
 */
LD_EXPORT(LDBoolean) LDObjectMerge(struct LDJSON *const to,
    const struct LDJSON *const from);

 /*@}*/

/***************************************************************************//**
 * @name Serialization / Deserialization
 * Working with textual representations of JSON
 * @{
 ******************************************************************************/

/**
 * @brief Serialize JSON structure into JSON text.
 * @param[in] json Structure to serialize.
 * May be `NULL`.
 * @return `NULL` on failure
 */
LD_EXPORT(char *) LDJSONSerialize(const struct LDJSON *const json);

/**
 * @brief Deserialize JSON text into a JSON structure.
 * @param[in] text JSON text to deserialize. May not be `NULL`.
 * @return JSON structure on success, `NULL` on failure.
 */
LD_EXPORT(struct LDJSON *) LDJSONDeserialize(const char *const text);

/*@}*/
