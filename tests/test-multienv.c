#include <stdio.h>

#include <launchdarkly/api.h>

#include "ldinternal.h"

int
main()
{
    struct LDClient *client, *secondary;
    struct LDConfig *config;
    struct LDUser *user;
    
    LDBasicLoggerThreadSafeInitialize();
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLoggerThreadSafe);

    LD_ASSERT(config = LDConfigNew("my-primary-key"));
    LDConfigSetOffline(config, true);
    LD_ASSERT(LDConfigAddSecondaryMobileKey(config, "secondary", "my-secondary-key"));
    LD_ASSERT(user = LDUserNew("my-user"));

    LD_ASSERT(client = LDClientInit(config, user, 0));
    LD_ASSERT(secondary = LDClientGetForMobileKey("secondary"));
    LD_ASSERT(client != secondary);
    LD_ASSERT(!LDClientGetForMobileKey("unknown environment"));

    LDClientClose(client);

    LDBasicLoggerThreadSafeShutdown();

    return 0;
}
