/*!
 * @file user.h
 * @brief Public API Interface for User construction
 */

#pragma once

#include <launchdarkly/json.h>
#include <launchdarkly/boolean.h>
#include <launchdarkly/export.h>

/** @brief Opaque user object **/
struct LDUser;

/** @brief Allocate a new user.
 *
 * The user may be modified *until* it is passed
 * to the `LDClientIdentify` or `LDClientInit`. The `key` argument is not
 * required. When `key` is `NULL` then a device specific ID is used. If a
 * device specific ID cannot be obtained then a random fallback is generated. */
LD_EXPORT(struct LDUser *) LDUserNew(const char *const key);

/** @brief Free a user object */
LD_EXPORT(void) LDUserFree(struct LDUser *const user);

/** @brief Mark the user as anonymous. */
LD_EXPORT(void) LDUserSetAnonymous(struct LDUser *const user,
    const LDBoolean anon);

/** @brief Set the user's IP. */
LD_EXPORT(LDBoolean) LDUserSetIP(struct LDUser *const user,
    const char *const str);

/** @brief Set the user's first name. */
LD_EXPORT(LDBoolean) LDUserSetFirstName(struct LDUser *const user,
    const char *const str);

/** @brief Set the user's last name. */
LD_EXPORT(LDBoolean) LDUserSetLastName(struct LDUser *const user,
    const char *const str);

/** @brief Set the user's email. */
LD_EXPORT(LDBoolean) LDUserSetEmail(struct LDUser *const user,
    const char *const str);

/** @brief Set the user's name. */
LD_EXPORT(LDBoolean) LDUserSetName(struct LDUser *const user,
    const char *const str);

/** @brief Set the user's avatar. */
LD_EXPORT(LDBoolean) LDUserSetAvatar(struct LDUser *const user,
    const char *const str);

/** @brief Set the user's secondary key. */
LD_EXPORT(LDBoolean) LDUserSetSecondary(struct LDUser *const user,
    const char *const str);

/** Set the user's private fields, must be array of strings */
LD_EXPORT(void) LDUserSetPrivateAttributes(struct LDUser *const user,
    struct LDJSON *const privateAttributes);

/** @brief Set the user's custom field */
LD_EXPORT(void) LDUserSetCustomAttributesJSON(struct LDUser *const user,
    struct LDJSON *const custom);
