#include <launchdarkly/api.h>

#include "config.h"
#include "ldinternal.h"

struct LDConfig *
LDConfigNew(const char *const mobileKey)
{
    struct LDConfig *config;

    LD_ASSERT_API(mobileKey);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (mobileKey == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigNew NULL mobileKey");

        return NULL;
    }
#endif

    LDi_once(&LDi_earlyonce, LDi_earlyinit);

    if (!(config = LDAlloc(sizeof(*config)))) {
        return NULL;
    }

    config->allAttributesPrivate            = LDBooleanFalse;
    config->backgroundPollingIntervalMillis = 15 * 60 * 1000;
    config->connectionTimeoutMillis         = 10 * 1000;
    config->requestTimeoutMillis            = 30 * 1000;
    config->disableBackgroundUpdating       = LDBooleanFalse;
    config->eventsCapacity                  = 100;
    config->eventsFlushIntervalMillis       = 30000;
    config->offline                         = LDBooleanFalse;
    config->pollingIntervalMillis           = 30 * 1000;
    config->privateAttributeNames           = NULL;
    config->streaming                       = LDBooleanTrue;
    config->useReport                       = LDBooleanFalse;
    config->useReasons                      = LDBooleanFalse;
    config->proxyURI                        = NULL;
    config->verifyPeer                      = LDBooleanTrue;
    config->certFile                        = NULL;
    config->inlineUsersInEvents             = LDBooleanFalse;
    config->appURI                          = NULL;
    config->eventsURI                       = NULL;
    config->mobileKey                       = NULL;
    config->streamURI                       = NULL;
    config->secondaryMobileKeys             = NULL;
    config->autoAliasOptOut                 = 0;

    if (!LDSetString(&config->appURI, "https://app.launchdarkly.com")) {
        goto error;
    }

    if (!LDSetString(&config->eventsURI, "https://mobile.launchdarkly.com")) {
        goto error;
    }

    if (!LDSetString(&config->mobileKey, mobileKey)) {
        goto error;
    }

    if (!LDSetString(
            &config->streamURI, "https://clientstream.launchdarkly.com")) {
        goto error;
    }

    if (!(config->secondaryMobileKeys = LDNewObject())) {
        goto error;
    }

    return config;

error:
    LDConfigFree(config);

    return NULL;
}

void
LDConfigSetAllAttributesPrivate(
    struct LDConfig *const config, const LDBoolean allPrivate)
{
    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetAllAttributesPrivate NULL config");

        return;
    }
#endif

    config->allAttributesPrivate = allPrivate;
}

void
LDConfigSetBackgroundPollingIntervalMillis(
    struct LDConfig *const config, const int millis)
{
    int minimum;

    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(
            LD_LOG_WARNING,
            "LDConfigSetBackgroundPollingIntervalMillis NULL config");

        return;
    }
#endif

    minimum = 15 * 60 * 1000;

    if (millis >= minimum) {
        config->backgroundPollingIntervalMillis = millis;
    } else {
        config->backgroundPollingIntervalMillis = minimum;
    }
}

LDBoolean
LDConfigSetAppURI(struct LDConfig *const config, const char *const uri)
{
    LD_ASSERT_API(config);
    LD_ASSERT_API(uri);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetAppURI NULL config");

        return LDBooleanFalse;
    }

    if (uri == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetAppURI NULL uri");

        return LDBooleanFalse;
    }
#endif

    return LDSetString(&config->appURI, uri);
}

void
LDConfigSetConnectionTimeoutMillis(
    struct LDConfig *const config, const int millis)
{
    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(
            LD_LOG_WARNING, "LDConfigSetConnectionTimeoutMillies NULL config");

        return;
    }
#endif

    config->connectionTimeoutMillis = millis;
}

void
LDConfigSetConnectionTimeoutMillies(
    struct LDConfig *const config, const int millis)
{
    LDConfigSetConnectionTimeoutMillis(config, millis);
}

void
LDConfigSetRequestTimeoutMillis(
        struct LDConfig *const config, const int millis)
{
    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(
                LD_LOG_WARNING, "LDConfigSetRequestTimeoutMillis NULL config");

        return;
    }
#endif

    config->requestTimeoutMillis = millis;
}

void
LDConfigSetDisableBackgroundUpdating(
    struct LDConfig *const config, const LDBoolean disable)
{
    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(
            LD_LOG_WARNING, "LDConfigSetDisableBackgroundUpdating NULL config");

        return;
    }
#endif

    config->disableBackgroundUpdating = disable;
}

void
LDConfigSetEventsCapacity(struct LDConfig *const config, const int capacity)
{
    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetEventsCapacity NULL config");

        return;
    }
#endif

    config->eventsCapacity = capacity;
}

void
LDConfigSetEventsFlushIntervalMillis(
    struct LDConfig *const config, const int millis)
{
    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(
            LD_LOG_WARNING, "LDConfigSetEventsFlushIntervalMillis NULL config");

        return;
    }
#endif

    config->eventsFlushIntervalMillis = millis;
}

LDBoolean
LDConfigSetEventsURI(struct LDConfig *const config, const char *const uri)
{
    LD_ASSERT_API(config);
    LD_ASSERT_API(uri);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetEventsURI NULL config");

        return LDBooleanFalse;
    }

    if (uri == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetEventsURI NULL uri");

        return LDBooleanFalse;
    }
#endif

    return LDSetString(&config->eventsURI, uri);
}

LDBoolean
LDConfigSetMobileKey(struct LDConfig *const config, const char *const key)
{
    LD_ASSERT_API(config);
    LD_ASSERT_API(key);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetMobleKey NULL config");

        return LDBooleanFalse;
    }

    if (key == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetMobileKey NULL key");

        return LDBooleanFalse;
    }
#endif

    return LDSetString(&config->mobileKey, key);
}

void
LDConfigSetOffline(struct LDConfig *const config, const LDBoolean offline)
{
    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetOffline NULL config");

        return;
    }
#endif

    config->offline = offline;
}

void
LDConfigSetStreaming(struct LDConfig *const config, const LDBoolean streaming)
{
    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetStreaming NULL config");

        return;
    }
#endif

    config->streaming = streaming;
}

void
LDConfigSetPollingIntervalMillis(
    struct LDConfig *const config, const int millis)
{
    int minimum;

    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetPollingIntervalMillis NULL config");

        return;
    }
#endif

    minimum = 30 * 1000;

    if (millis >= minimum) {
        config->pollingIntervalMillis = millis;
    } else {
        config->pollingIntervalMillis = minimum;
    }
}

LDBoolean
LDConfigSetStreamURI(struct LDConfig *const config, const char *const uri)
{
    LD_ASSERT_API(config);
    LD_ASSERT_API(uri);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetStreamURI NULL config");

        return LDBooleanFalse;
    }

    if (uri == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetStreamURI NULL uri");

        return LDBooleanFalse;
    }
#endif

    return LDSetString(&config->streamURI, uri);
}

void
LDConfigSetUseReport(struct LDConfig *const config, const LDBoolean report)
{
    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetUseReport NULL config");

        return;
    }
#endif

    config->useReport = report;
}

void
LDConfigSetUseEvaluationReasons(
    struct LDConfig *const config, const LDBoolean reasons)
{
    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetUseEvaluationReasons NULL config");

        return;
    }
#endif

    config->useReasons = reasons;
}

LDBoolean
LDConfigSetProxyURI(struct LDConfig *const config, const char *const uri)
{
    LD_ASSERT_API(config);
    LD_ASSERT_API(uri);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetProxyURI NULL config");

        return LDBooleanFalse;
    }

    if (uri == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetProxyURI NULL uri");

        return LDBooleanFalse;
    }
#endif

    return LDSetString(&config->proxyURI, uri);
}

void
LDConfigSetVerifyPeer(struct LDConfig *const config, const LDBoolean enabled)
{
    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetVerifyPeer NULL config");

        return;
    }
#endif

    config->verifyPeer = enabled;
}

LDBoolean
LDConfigSetSSLCertificateAuthority(
    struct LDConfig *const config, const char *const certFile)
{
    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(
            LD_LOG_WARNING, "LDConfigSetSSLCertificateAuthority NULL config");

        return LDBooleanFalse;
    }

    if (certFile == NULL) {
        LD_LOG(
            LD_LOG_WARNING, "LDConfigSetSSLCertificateAuthority NULL certFile");

        return LDBooleanFalse;
    }
#endif

    return LDSetString(&config->certFile, certFile);
}

void
LDConfigSetPrivateAttributes(
    struct LDConfig *const config, struct LDJSON *attributes)
{
    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetPrivateAttributes NULL config");

        return;
    }
#endif

    if (attributes) {
        LD_ASSERT(LDJSONGetType(attributes) == LDArray);

        LDJSONFree(config->privateAttributeNames);
    }

    config->privateAttributeNames = attributes;
}

LDBoolean
LDConfigAddSecondaryMobileKey(
    struct LDConfig *const config,
    const char *const      name,
    const char *const      key)
{
    struct LDJSON *tmp;

    LD_ASSERT_API(config);
    LD_ASSERT_API(name);
    LD_ASSERT_API(key);

    if (strcmp(name, LDPrimaryEnvironmentName) == 0) {
        LD_LOG(
            LD_LOG_ERROR,
            "Attempted use the primary environment name as secondary");

        return LDBooleanFalse;
    }

    if (strcmp(key, config->mobileKey) == 0) {
        LD_LOG(LD_LOG_ERROR, "Attempted to add primary key as secondary key");

        return LDBooleanFalse;
    }

    tmp = LDObjectLookup(config->secondaryMobileKeys, name);

    if (tmp && strcmp(LDGetText(tmp), name) == 0) {
        LD_LOG(LD_LOG_ERROR, "Attempted to add secondary key twice");

        return LDBooleanFalse;
    }

    if (!(tmp = LDNewText(key))) {
        LD_LOG(
            LD_LOG_ERROR,
            "LDConfigAddSecondaryMobileKey failed to duplicate key");

        return LDBooleanFalse;
    }

    if (!LDObjectSetKey(config->secondaryMobileKeys, name, tmp)) {
        LDJSONFree(tmp);

        LD_LOG(
            LD_LOG_ERROR,
            "LDConfigAddSecondaryMobileKey failed to add environment");

        return LDBooleanFalse;
    }

    return LDBooleanTrue;
}

void
LDConfigSetInlineUsersInEvents(
    struct LDConfig *const config, const LDBoolean inlineUsers)
{
    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigSetInlineUsersInEvents NULL config");

        return;
    }
#endif

    config->inlineUsersInEvents = inlineUsers;
}

void
LDConfigAutoAliasOptOut(struct LDConfig *const config, const LDBoolean optOut)
{
    LD_ASSERT_API(config);

#ifdef LAUNCHDARKLY_DEFENSIVE
    if (config == NULL) {
        LD_LOG(LD_LOG_WARNING, "LDConfigAutoAliasOptOut NULL config");

        return;
    }
#endif

    config->autoAliasOptOut = optOut;
}

void
LDConfigFree(struct LDConfig *const config)
{
    if (config) {
        LDFree(config->appURI);
        LDFree(config->eventsURI);
        LDFree(config->mobileKey);
        LDFree(config->streamURI);
        LDFree(config->proxyURI);
        LDFree(config->certFile);
        LDJSONFree(config->privateAttributeNames);
        LDJSONFree(config->secondaryMobileKeys);
        LDFree(config);
    }
}
