#include <stdio.h>

#include "ldapi.h"
#include "ldinternal.h"

int
main()
{
    LDClient *client, *secondary;
    LDConfig *config;
    LDUser *user;

    LD_ASSERT(config = LDConfigNew("my-primary-key"));
    LDConfigSetOffline(config, true);
    LD_ASSERT(LDConfigAddSecondaryMobileKey(config, "secondary", "my-secondary-key"));
    LD_ASSERT(user = LDUserNew("my-user"));

    LD_ASSERT(client = LDClientInit(config, user, 0));
    LD_ASSERT(secondary = LDClientGetForMobileKey("secondary"));
    LD_ASSERT(client != secondary);
    LD_ASSERT(!LDClientGetForMobileKey("unknown environment"));

    LDClientClose(client);

    return 0;
}
