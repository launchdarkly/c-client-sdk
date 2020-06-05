#pragma once

#include <launchdarkly/json.h>

struct LDUser {
    char *key;
    bool anonymous;
    char *secondary;
    char *ip;
    char *firstName;
    char *lastName;
    char *email;
    char *name;
    char *avatar;
    struct LDJSON *privateAttributeNames;
    struct LDJSON *custom;
};

char *LDi_usertojsontext(
    const struct LDClient *const client,
    const struct LDUser *const   lduser,
    const bool                   redact
);

struct LDJSON *
LDi_userToJSON(
    const struct LDConfig *const config,
    const struct LDUser *const   lduser,
    const bool                   redact
);
