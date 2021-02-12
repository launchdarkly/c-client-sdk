#include <string.h>

#include <launchdarkly/api.h>

#include "assertion.h"
#include "config.h"

void
testNullKey()
{
    LD_ASSERT(LDConfigNew(NULL) == NULL);
}

void
testSetters()
{
    struct LDConfig *config;
    struct LDJSON *  attributes, *tmp;

    LD_ASSERT(config = LDConfigNew("a"));
    LD_ASSERT(strcmp("a", config->mobileKey) == 0);

    LDConfigSetAllAttributesPrivate(config, true);
    LD_ASSERT(config->allAttributesPrivate == true);

    /* respects minimum */
    LDConfigSetBackgroundPollingIntervalMillis(config, 5);
    LD_ASSERT(config->backgroundPollingIntervalMillis == 15 * 60 * 1000);

    LDConfigSetBackgroundPollingIntervalMillis(config, 20 * 60 * 1000);
    LD_ASSERT(config->backgroundPollingIntervalMillis == 20 * 60 * 1000);

    LD_ASSERT(LDConfigSetAppURI(config, "https://test1.com"));
    LD_ASSERT(strcmp("https://test1.com", config->appURI) == 0);

    LDConfigSetConnectionTimeoutMillies(config, 52);
    LD_ASSERT(config->connectionTimeoutMillis == 52);

    LDConfigSetDisableBackgroundUpdating(config, true);
    LD_ASSERT(config->disableBackgroundUpdating == true);

    LDConfigSetEventsCapacity(config, 12);
    LD_ASSERT(config->eventsCapacity == 12);

    LDConfigSetEventsFlushIntervalMillis(config, 66);
    LD_ASSERT(config->eventsFlushIntervalMillis == 66);

    LD_ASSERT(LDConfigSetEventsURI(config, "https://test2.com"));
    LD_ASSERT(strcmp("https://test2.com", config->eventsURI) == 0);

    LD_ASSERT(LDConfigSetMobileKey(config, "key1"));
    LD_ASSERT(strcmp("key1", config->mobileKey) == 0);

    LDConfigSetOffline(config, true);
    LD_ASSERT(config->offline == true);

    LDConfigSetStreaming(config, false);
    LD_ASSERT(config->streaming == false);

    /* respects minimum */
    LDConfigSetPollingIntervalMillis(config, 5);
    LD_ASSERT(config->pollingIntervalMillis == 30 * 1000);

    LDConfigSetPollingIntervalMillis(config, 50 * 1000);
    LD_ASSERT(config->pollingIntervalMillis == 50 * 1000);

    LD_ASSERT(LDConfigSetStreamURI(config, "https://test3.com"));
    LD_ASSERT(strcmp("https://test3.com", config->streamURI) == 0);

    LDConfigSetUseReport(config, true);
    LD_ASSERT(config->useReport == true);

    LDConfigSetUseEvaluationReasons(config, true);
    LD_ASSERT(config->useReasons == true);

    LD_ASSERT(LDConfigSetProxyURI(config, "https://test4.com"));
    LD_ASSERT(strcmp("https://test4.com", config->proxyURI) == 0);

    LDConfigSetVerifyPeer(config, false);
    LD_ASSERT(config->verifyPeer == false);

    LD_ASSERT(LDConfigSetSSLCertificateAuthority(config, "/path"));
    LD_ASSERT(strcmp("/path", config->certFile) == 0);

    LD_ASSERT(LDConfigAddSecondaryMobileKey(config, "name", "key"));

    LD_ASSERT(attributes = LDNewArray());
    LD_ASSERT(tmp = LDNewText("name"));
    LD_ASSERT(LDArrayPush(attributes, tmp));
    LDConfigSetPrivateAttributes(config, attributes);

    LDConfigSetInlineUsersInEvents(config, true);
    LD_ASSERT(config->inlineUsersInEvents == true);

    LDConfigFree(config);
}

int
main()
{
    LDBasicLoggerThreadSafeInitialize();
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLoggerThreadSafe);

    testNullKey();
    testSetters();

    LDBasicLoggerThreadSafeShutdown();

    return 0;
}
