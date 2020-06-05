#pragma once

#include <launchdarkly/json.h>

struct LDConfig {
    bool allAttributesPrivate;
    int backgroundPollingIntervalMillis;
    char *appURI;
    int connectionTimeoutMillis;
    bool disableBackgroundUpdating;
    unsigned int eventsCapacity;
    int eventsFlushIntervalMillis;
    char *eventsURI;
    char *mobileKey;
    bool offline;
    int pollingIntervalMillis;
    bool streaming;
    char *streamURI;
    bool useReport;
    char *proxyURI;
    bool verifyPeer;
    bool useReasons;
    char *certFile;
    bool inlineUsersInEvents;
    /* map of name -> key */
    struct LDJSON *secondaryMobileKeys;
    /* array of strings */
    struct LDJSON *privateAttributeNames;
};
