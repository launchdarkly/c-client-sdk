#include <string.h>

#include <launchdarkly/api.h>

#include "assertion.h"
#include "client.h"

void
callback(int status)
{
}

void
testClientMisc()
{
    struct LDUser *  user;
    struct LDConfig *config;
    struct LDClient *client;

    LDSetClientStatusCallback(callback);

    LD_ASSERT(user = LDUserNew("a"));
    LD_ASSERT(config = LDConfigNew("b"));
    LD_ASSERT(client = LDClientInit(config, user, 0));

    LD_ASSERT(LDClientIsInitialized(client) == false);

    LD_ASSERT(client == LDClientGet());

    LDClientSetBackground(client, true);
    LDClientSetBackground(client, false);

    LD_ASSERT(client->offline == false);
    LD_ASSERT(LDClientIsOffline(client) == false);
    LDClientSetOffline(client);
    LD_ASSERT(LDClientIsOffline(client) == true);
    LD_ASSERT(client->offline == true);
    LDClientSetOnline(client);
    LD_ASSERT(LDClientIsOffline(client) == false);
    LD_ASSERT(client->offline == false);

    LDClientClose(client);
}

int
main()
{
    LDBasicLoggerThreadSafeInitialize();
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLoggerThreadSafe);

    testClientMisc();

    LDBasicLoggerThreadSafeShutdown();

    return 0;
}
