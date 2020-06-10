/*!
 * @file memory.h
 * @brief Public API. Operations for managing memory.
 */

#pragma once

#include <stddef.h>

#include <launchdarkly/export.h>

/** @brief Equivalent to `malloc` */
LD_EXPORT(void *) LDAlloc(const size_t bytes);
/** @brief Equivalent to `free` */
LD_EXPORT(void) LDFree(void *const buffer);
/** @brief Equivalent to `strdup` */
LD_EXPORT(char *) LDStrDup(const char *const string);
/** @brief Equivalent to `realloc` */
LD_EXPORT(void *) LDRealloc(void *const buffer, const size_t bytes);
/** @brief Equivalent to `calloc` */
LD_EXPORT(void *) LDCalloc(const size_t nmemb, const size_t size);
/** @brief Equivalent to `strndup` */
LD_EXPORT(char *) LDStrNDup(const char *const str, const size_t n);

/** @brief Set all the memory related functions to be used by the SDK */
LD_EXPORT(void) LDSetMemoryRoutines(void *(*const newMalloc)(const size_t),
    void (*const newFree)(void *const),
    void *(*const newRealloc)(void *const, const size_t),
    char *(*const newStrDup)(const char *const),
    void *(*const newCalloc)(const size_t, const size_t),
    char *(*const newStrNDup)(const char *const, const size_t));

/** @brief Must be called once before any other API function */
LD_EXPORT(void) LDGlobalInit(void);
