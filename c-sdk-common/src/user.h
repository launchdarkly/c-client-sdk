#pragma once

#include <launchdarkly/json.h>

struct LDUser
{
    char *         key;
    LDBoolean      anonymous;
    char *         secondary;
    char *         ip;
    char *         firstName;
    char *         lastName;
    char *         email;
    char *         name;
    char *         avatar;
    char *         country;
    struct LDJSON *privateAttributeNames; /* Array of Text */
    struct LDJSON *custom;                /* Object, may be NULL */
};

struct LDJSON *
LDi_valueOfAttribute(
    const struct LDUser *const user, const char *const attribute);

/* Creates an "event user", which is a representation of a user suitable
 * for events. Attributes are redacted according to allAttributesPrivate, and
 * the list of global private attribute names.
 * The returned LDJSON must be freed with LDJSONFree. */
struct LDJSON *
LDi_createEventUser(
    const struct LDUser *user,
    LDBoolean            allAttributesPrivate,
    const struct LDJSON *globalPrivateAttributeNames);


/* Serializes a user to its JSON representation. The returned string must be
 * freed with LDFree. */
char *
LDi_serializeUser(const struct LDUser *user);

/* Server and client have different semantics for handling of a NULL user key.
because of this LDUserNew is not defined in c-sdk-common. This function
is the part of the internal implementation for LDUserNew */
struct LDUser *
LDi_userNew(const char *const key);
