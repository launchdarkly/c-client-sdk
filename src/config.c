#include "ldapi.h"
#include "ldinternal.h"
#include "config.h"

struct LDConfig *
LDConfigNew(const char *const mobileKey)
{
    struct LDConfig *config;

    LD_ASSERT(mobileKey);

    LDi_once(&LDi_earlyonce, LDi_earlyinit);

    if (!(config = LDAlloc(sizeof(*config)))) {
        return NULL;
    }

    memset(config, 0, sizeof(*config));

    LD_ASSERT(LDSetString(&config->appURI,
        "https://app.launchdarkly.com"));
    LD_ASSERT(LDSetString(&config->eventsURI,
        "https://mobile.launchdarkly.com"));
    LD_ASSERT(LDSetString(&config->mobileKey, mobileKey));
    LD_ASSERT(LDSetString(&config->streamURI,
        "https://clientstream.launchdarkly.com"));
    LD_ASSERT(config->secondaryMobileKeys = LDNewObject());

    config->allAttributesPrivate            = false;
    config->backgroundPollingIntervalMillis = 15 * 60 * 1000;
    config->connectionTimeoutMillis         = 10000;
    config->disableBackgroundUpdating       = false;
    config->eventsCapacity                  = 100;
    config->eventsFlushIntervalMillis       = 30000;
    config->offline                         = false;
    config->pollingIntervalMillis           = 30 * 1000;
    config->privateAttributeNames           = NULL;
    config->streaming                       = true;
    config->useReport                       = false;
    config->useReasons                      = false;
    config->proxyURI                        = NULL;
    config->verifyPeer                      = true;
    config->certFile                        = NULL;
    config->inlineUsersInEvents             = false;

    return config;
}

void
LDConfigSetAllAttributesPrivate(struct LDConfig *const config,
    const bool private)
{
    LD_ASSERT(config);

    config->allAttributesPrivate = private;
}

void
LDConfigSetBackgroundPollingIntervalMillis(struct LDConfig *const config,
    const int millis)
{
    int minimum;

    LD_ASSERT(config);

    minimum = 15 * 60 * 1000;

    if (millis >= minimum) {
        config->backgroundPollingIntervalMillis = millis;
    } else {
        config->backgroundPollingIntervalMillis = minimum;
    }
}

void
LDConfigSetAppURI(struct LDConfig *const config, const char *const uri)
{
    LD_ASSERT(config);
    LD_ASSERT(uri);

    LD_ASSERT(LDSetString(&config->appURI, uri));
}

void
LDConfigSetConnectionTimeoutMillies(struct LDConfig *const config,
    const int millis)
{
    LD_ASSERT(config);

    config->connectionTimeoutMillis = millis;
}

void
LDConfigSetDisableBackgroundUpdating(struct LDConfig *const config,
    const bool disable)
{
    LD_ASSERT(config);

    config->disableBackgroundUpdating = disable;
}

void
LDConfigSetEventsCapacity(struct LDConfig *const config, const int capacity)
{
    LD_ASSERT(config);

    config->eventsCapacity = capacity;
}

void
LDConfigSetEventsFlushIntervalMillis(struct LDConfig *const config,
    const int millis)
{
    LD_ASSERT(config);

    config->eventsFlushIntervalMillis = millis;
}

void
LDConfigSetEventsURI(struct LDConfig *const config, const char *const uri)
{
    LD_ASSERT(config);
    LD_ASSERT(uri);

    LD_ASSERT(LDSetString(&config->eventsURI, uri));
}

void
LDConfigSetMobileKey(struct LDConfig *const config, const char *const key)
{
    LD_ASSERT(config);
    LD_ASSERT(key);

    LD_ASSERT(LDSetString(&config->mobileKey, key));
}

void
LDConfigSetOffline(struct LDConfig *const config, const bool offline)
{
    LD_ASSERT(config);

    config->offline = offline;
}

void
LDConfigSetStreaming(struct LDConfig *const config, const bool streaming)
{
    LD_ASSERT(config);

    config->streaming = streaming;
}

void
LDConfigSetPollingIntervalMillis(struct LDConfig *const config,
    const int millis)
{
    int minimum;

    LD_ASSERT(config);

    minimum = 30 * 1000;

    if (millis >= minimum) {
        config->pollingIntervalMillis = millis;
    } else {
        config->pollingIntervalMillis = minimum;
    }
}

void
LDConfigSetStreamURI(struct LDConfig *const config, const char *const uri)
{
    LD_ASSERT(config);
    LD_ASSERT(uri);

    LD_ASSERT(LDSetString(&config->streamURI, uri));
}

void
LDConfigSetUseReport(struct LDConfig *const config, const bool report)
{
    LD_ASSERT(config);

    config->useReport = report;
}

void
LDConfigSetUseEvaluationReasons(struct LDConfig *const config,
    const bool reasons)
{
    LD_ASSERT(config);

    config->useReasons = reasons;
}

void
LDConfigSetProxyURI(struct LDConfig *const config, const char *const uri)
{
    LD_ASSERT(config);
    LD_ASSERT(uri);

    LD_ASSERT(LDSetString(&config->proxyURI, uri));
}

void
LDConfigSetVerifyPeer(struct LDConfig *const config, const bool enabled)
{
    LD_ASSERT(config);

    config->verifyPeer = enabled;
}

void
LDConfigSetSSLCertificateAuthority(struct LDConfig *const config,
    const char *const certFile)
{
    LD_ASSERT(config);

    LD_ASSERT(LDSetString(&config->certFile, certFile));
}

void
LDConfigSetPrivateAttributes(struct LDConfig *const config,
    struct LDJSON *attributes)
{
    LD_ASSERT(config);

    if (attributes) {
        LD_ASSERT(LDJSONGetType(attributes) == LDArray);

        LDJSONFree(config->privateAttributeNames);
    }

    config->privateAttributeNames = attributes;
}

bool
LDConfigAddSecondaryMobileKey(struct LDConfig *const config,
    const char *const name, const char *const key)
{
    struct LDJSON *tmp;

    LD_ASSERT(config);
    LD_ASSERT(name);
    LD_ASSERT(key);

    if (strcmp(name, LDPrimaryEnvironmentName) == 0) {
        LD_LOG(LD_LOG_ERROR,
            "Attempted use the primary environment name as secondary");

        return false;
    }

    if (strcmp(key, config->mobileKey) == 0) {
        LD_LOG(LD_LOG_ERROR,
            "Attempted to add primary key as secondary key");

        return false;
    }

    tmp = LDObjectLookup(config->secondaryMobileKeys, name);

    if (tmp && strcmp(LDGetText(tmp), name) == 0) {
        LD_LOG(LD_LOG_ERROR, "Attempted to add secondary key twice");

        return false;
    }

    if (!(tmp = LDNewText(key))) {
        LD_LOG(LD_LOG_ERROR,
            "LDConfigAddSecondaryMobileKey failed to duplicate key");

        return false;
    }

    if (!LDObjectSetKey(config->secondaryMobileKeys, name, tmp)) {
        LDJSONFree(tmp);

        LD_LOG(LD_LOG_ERROR,
            "LDConfigAddSecondaryMobileKey failed to add environment");

        return false;
    }

    return true;
}

void
LDConfigSetInlineUsersInEvents(struct LDConfig *const config,
    const bool inlineUsers)
{
    LD_ASSERT(config);

    config->inlineUsersInEvents = inlineUsers;
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
