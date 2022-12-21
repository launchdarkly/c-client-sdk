/*!
 * @file user.h
 * @brief Public API Interface for User construction
 */

#pragma once

#include <launchdarkly/boolean.h>
#include <launchdarkly/export.h>
#include <launchdarkly/json.h>

/**
 * @struct LDUser
 * @brief An opaque user object
 */
struct LDUser;

/**
 * @brief Allocate a new empty user Object.
 * @param[in] key A string that identifies the user. For c-server-sdk
 * this value may not be NULL. For c-client-sdk if a NULL key is provided
 * the SDK will use a persistent platform identifier. If a identifier
 * cannot be determined a non persistent fallback will be generated.
 * @return `NULL` on failure.
 */
LD_EXPORT(struct LDUser *) LDUserNew(const char *const key);

/**
 * @brief Destroy an existing user object.
 * @param[in] user The user free. May be `NULL`.
 * @return Void.
 */
LD_EXPORT(void) LDUserFree(struct LDUser *const user);

/**
 * @brief Mark the user as anonymous.
 * @param[in] user The user to mutate. May not be `NULL`.
 * @param[in] anon If the user should be anonymous or not.
 * @return Void.
 */
LD_EXPORT(void)
LDUserSetAnonymous(struct LDUser *const user, const LDBoolean anon);

/**
 * @brief Set the user's IP.
 * @param[in] user The user to mutate. May not be `NULL`.
 * @param[in] ip The user's IP. May be `NULL`.
 * @return True on success, False on failure.
 */
LD_EXPORT(LDBoolean)
LDUserSetIP(struct LDUser *const user, const char *const ip);

/**
 * @brief Set the user's first name.
 * @param[in] user The user to mutate. May not be `NULL`.
 * @param[in] firstName The user's first name. May be `NULL`.
 * @return True on success, False on failure.
 */
LD_EXPORT(LDBoolean)
LDUserSetFirstName(struct LDUser *const user, const char *const firstName);

/**
 * @brief Set the user's last name.
 * @param[in] user The user to mutate. May not be `NULL`.
 * @param[in] lastName The user's last name. May be `NULL`.
 * @return True on success, False on failure.
 */
LD_EXPORT(LDBoolean)
LDUserSetLastName(struct LDUser *const user, const char *const lastName);

/**
 * @brief Set the user's email.
 * @param[in] user The user to mutate. May not be `NULL`.
 * @param[in] email The user's email. May be `NULL`.
 * @return True on success, False on failure.
 */
LD_EXPORT(LDBoolean)
LDUserSetEmail(struct LDUser *const user, const char *const email);

/**
 * @brief Set the user's name.
 * @param[in] user The user to mutate. May not be `NULL`.
 * @param[in] name The user's name. May be `NULL`.
 * @return True on success, False on failure.
 */
LD_EXPORT(LDBoolean)
LDUserSetName(struct LDUser *const user, const char *const name);

/**
 * @brief Set the user's avatar.
 * @param[in] user The user to mutate. May not be `NULL`.
 * @param[in] avatar The user's avatar. May be `NULL`.
 * @return True on success, False on failure.
 */
LD_EXPORT(LDBoolean)
LDUserSetAvatar(struct LDUser *const user, const char *const avatar);

/**
 * @brief Set the user's country.
 * @param[in] user The user to mutate. May not be `NULL`.
 * @param[in] country The user's country. May be `NULL`.
 * @return True on success, False on failure.
 */
LD_EXPORT(LDBoolean)
LDUserSetCountry(struct LDUser *const user, const char *const country);

/**
 * @brief Set the user's secondary key.
 * @param[in] user The user to mutate. May not be `NULL`.
 * @param[in] secondary The user's secondary key. May be `NULL`.
 * @return True on success, False on failure.
 */
LD_EXPORT(LDBoolean)
LDUserSetSecondary(struct LDUser *const user, const char *const secondary);

/**
 * @brief Set the user's custom JSON.
 * @param[in] user The user to mutate. May not be `NULL`.
 * @param[in] custom Custom JSON for the user. May be `NULL`.
 * @return Void.
 */
LD_EXPORT(void)
LDUserSetCustom(struct LDUser *const user, struct LDJSON *const custom);

/**
 * @brief Mark an attribute as private.
 * @param[in] user The user to mutate. May not be `NULL`.
 * @param[in] attribute Attribute to mark as private. May not be `NULL`.
 * @return True on success, False on failure.
 */
LD_EXPORT(LDBoolean)
LDUserAddPrivateAttribute(
    struct LDUser *const user, const char *const attribute);

/**
 * @brief Mark a set of attributes as private.
 * @param[in] user The user to mutate. May not be `NULL`.
 * @param[in] attribute The set of attributes to mark as private. Must be an
 * array of strings. May not be `NULL`.
 * @return Void.
 */
LD_EXPORT(void)
LDUserSetPrivateAttributes(
    struct LDUser *const user, struct LDJSON *const privateAttributes);

/**
 * @brief Set the user's custom field
 * @deprecated This is deprecated in favor of `LDUserSetCustom`.
 */
LD_EXPORT(void)
LDUserSetCustomAttributesJSON(
    struct LDUser *const user, struct LDJSON *const custom);
