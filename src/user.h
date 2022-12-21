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

struct LDJSON *
LDi_userToJSON(
    const struct LDUser *const user,
    const LDBoolean            redact,
    const LDBoolean            allAttributesPrivate,
    const struct LDJSON *const globalPrivateAttributeNames);

/* Server and client have different semantics for handling of a NULL user key.
because of this LDUserNew is not defined in c-sdk-common. This function
is the part of the internal implementation for LDUserNew */
struct LDUser *
LDi_userNew(const char *const key);
