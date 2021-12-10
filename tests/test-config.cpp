#include "gtest/gtest.h"
#include "commonfixture.h"

extern "C" {
#include <launchdarkly/api.h>

#include "config.h"
}

// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class ConfigFixture : public CommonFixture {
};

TEST_F(ConfigFixture, NullKey) {
    ASSERT_EQ(LDConfigNew(NULL), nullptr);
}

TEST_F(ConfigFixture, Setters) {
    struct LDConfig *config;
    struct LDJSON *attributes, *tmp;

    ASSERT_TRUE(config = LDConfigNew("a"));
    ASSERT_STREQ("a", config->mobileKey);

    LDConfigSetAllAttributesPrivate(config, LDBooleanTrue);
    ASSERT_TRUE(config->allAttributesPrivate);

    /* respects minimum */
    LDConfigSetBackgroundPollingIntervalMillis(config, 5);
    ASSERT_EQ(config->backgroundPollingIntervalMillis, 15 * 60 * 1000);

    LDConfigSetBackgroundPollingIntervalMillis(config, 20 * 60 * 1000);
    ASSERT_EQ(config->backgroundPollingIntervalMillis, 20 * 60 * 1000);

    ASSERT_TRUE(LDConfigSetAppURI(config, "https://test1.com"));
    ASSERT_STREQ("https://test1.com", config->appURI);

    LDConfigSetConnectionTimeoutMillies(config, 52);
    ASSERT_EQ(config->connectionTimeoutMillis, 52);

    LDConfigSetDisableBackgroundUpdating(config, LDBooleanTrue);
    ASSERT_TRUE(config->disableBackgroundUpdating);

    LDConfigSetEventsCapacity(config, 12);
    ASSERT_EQ(config->eventsCapacity, 12);

    LDConfigSetEventsFlushIntervalMillis(config, 66);
    ASSERT_EQ(config->eventsFlushIntervalMillis, 66);

    ASSERT_TRUE(LDConfigSetEventsURI(config, "https://test2.com"));
    ASSERT_STREQ("https://test2.com", config->eventsURI);

    ASSERT_TRUE(LDConfigSetMobileKey(config, "key1"));
    ASSERT_STREQ("key1", config->mobileKey);

    LDConfigSetOffline(config, LDBooleanTrue);
    ASSERT_TRUE(config->offline);

    LDConfigSetStreaming(config, LDBooleanFalse);
    ASSERT_FALSE(config->streaming);

    /* respects minimum */
    LDConfigSetPollingIntervalMillis(config, 5);
    ASSERT_EQ(config->pollingIntervalMillis, 30 * 1000);

    LDConfigSetPollingIntervalMillis(config, 50 * 1000);
    ASSERT_EQ(config->pollingIntervalMillis, 50 * 1000);

    ASSERT_TRUE(LDConfigSetStreamURI(config, "https://test3.com"));
    ASSERT_STREQ("https://test3.com", config->streamURI);

    LDConfigSetUseReport(config, LDBooleanTrue);
    ASSERT_TRUE(config->useReport);

    LDConfigSetUseEvaluationReasons(config, LDBooleanTrue);
    ASSERT_TRUE(config->useReasons);

    ASSERT_TRUE(LDConfigSetProxyURI(config, "https://test4.com"));
    ASSERT_STREQ("https://test4.com", config->proxyURI);

    LDConfigSetVerifyPeer(config, LDBooleanFalse);
    ASSERT_FALSE(config->verifyPeer);

    ASSERT_TRUE(LDConfigSetSSLCertificateAuthority(config, "/path"));
    ASSERT_STREQ("/path", config->certFile);

    ASSERT_TRUE(LDConfigAddSecondaryMobileKey(config, "name", "key"));

    ASSERT_TRUE(attributes = LDNewArray());
    ASSERT_TRUE(tmp = LDNewText("name"));
    ASSERT_TRUE(LDArrayPush(attributes, tmp));
    LDConfigSetPrivateAttributes(config, attributes);

    LDConfigSetInlineUsersInEvents(config, LDBooleanTrue);
    ASSERT_TRUE(config->inlineUsersInEvents);

    LDConfigFree(config);
}
