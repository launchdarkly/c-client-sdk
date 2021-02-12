#include <launchdarkly/api.h>

#include "ldinternal.h"

struct LDUser *
LDUserNew(const char *const key)
{
    struct LDUser *user;
    char           fallback[32 + 1] = {0};
    const char *   actualKey;
    char *         deviceIdentifier;
    LDBoolean      anonymous;

    deviceIdentifier = NULL;

    LDi_once(&LDi_earlyonce, LDi_earlyinit);

    if (!key || key[0] == '\0') {
        deviceIdentifier = LDi_deviceid();

        if (deviceIdentifier) {
            actualKey = deviceIdentifier;
        } else {
            LD_LOG(
                LD_LOG_WARNING,
                "Failed to get device ID falling back to random ID");

            LDi_randomhex(fallback, sizeof(fallback) - 1);

            actualKey = fallback;
        }

        anonymous = LDBooleanTrue;
    } else {
        actualKey = key;
        anonymous = LDBooleanFalse;
    }

    LD_LOG_1(LD_LOG_INFO, "Setting user key to: %s", actualKey);

    if (!(user = LDi_userNew(actualKey))) {
        LDFree(deviceIdentifier);

        return NULL;
    }

    LDFree(deviceIdentifier);

    if (anonymous) {
        LDUserSetAnonymous(user, LDBooleanTrue);
    }

    return user;
}
