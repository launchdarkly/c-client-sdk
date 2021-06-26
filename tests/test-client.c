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

    LD_ASSERT(LDClientIsInitialized(client) == LDBooleanFalse);

    LD_ASSERT(client == LDClientGet());

    LDClientSetBackground(client, LDBooleanTrue);
    LDClientSetBackground(client, LDBooleanFalse);

    LD_ASSERT(client->offline == LDBooleanFalse);
    LD_ASSERT(LDClientIsOffline(client) == LDBooleanFalse);
    LDClientSetOffline(client);
    LD_ASSERT(LDClientIsOffline(client) == LDBooleanTrue);
    LD_ASSERT(client->offline == LDBooleanTrue);
    LDClientSetOnline(client);
    LD_ASSERT(LDClientIsOffline(client) == LDBooleanFalse);
    LD_ASSERT(client->offline == LDBooleanFalse);

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
