#include "ldapi.h"
#include "ldinternal.h"

static struct LDClient *
makeTestClient()
{
    struct LDConfig *config;
    struct LDUser *user;
    struct LDClient *client;

    LD_ASSERT(config = LDConfigNew("abc"));
    LDConfigSetOffline(config, true);

    LD_ASSERT(user = LDUserNew("test-user"));

    LD_ASSERT(client = LDClientInit(config, user, 0));

    return client;
}

static void
testRestoreAndSaveEmpty()
{
    struct LDClient *client;
    char *bundle;

    LD_ASSERT(client = makeTestClient());

    LD_ASSERT(bundle = LDClientSaveFlags(client));
    LD_ASSERT(LDClientRestoreFlags(client, bundle));

    LDFree(bundle);

    LDClientClose(client);
}

static void
testRestoreAndSaveBasic()
{
    struct LDClient *client;
    char *bundle1, *bundle2;
    struct LDFlag flag;

    LD_ASSERT(client = makeTestClient());

    flag.key                  = LDStrDup("test");
    flag.value                = LDNewBool(true);
    flag.version              = 2;
    flag.flagVersion          = -1;
    flag.variation            = 3;
    flag.trackEvents          = false;
    flag.reason               = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted              = false;

    LD_ASSERT(LDi_storeUpsert(&client->store, flag));

    LD_ASSERT(bundle1 = LDClientSaveFlags(client));

    LDi_storeFreeFlags(&client->store);

    LD_ASSERT(LDClientRestoreFlags(client, bundle1));

    LD_ASSERT(bundle2 = LDClientSaveFlags(client));

    LD_ASSERT(strcmp(bundle1, bundle2) == 0);

    LDFree(bundle1);
    LDFree(bundle2);

    LDClientClose(client);
}

int
main(int argc, char **argv)
{
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLogger);

    testRestoreAndSaveEmpty();
    testRestoreAndSaveBasic();

    return 0;
}
