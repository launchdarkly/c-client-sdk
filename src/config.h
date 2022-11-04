#pragma once

#include <launchdarkly/boolean.h>
#include <launchdarkly/json.h>

struct LDConfig
{
    LDBoolean    allAttributesPrivate;
    int          backgroundPollingIntervalMillis;
    char *       appURI;
    int          connectionTimeoutMillis;
    int          requestTimeoutMillis;
    LDBoolean    disableBackgroundUpdating;
    unsigned int eventsCapacity;
    int          eventsFlushIntervalMillis;
    char *       eventsURI;
    char *       mobileKey;
    LDBoolean    offline;
    int          pollingIntervalMillis;
    LDBoolean    streaming;
    char *       streamURI;
    LDBoolean    useReport;
    char *       proxyURI;
    LDBoolean    verifyPeer;
    LDBoolean    useReasons;
    char *       certFile;
    LDBoolean    inlineUsersInEvents;
    LDBoolean    autoAliasOptOut;
    /* map of name -> key */
    struct LDJSON *secondaryMobileKeys;
    /* array of strings */
    struct LDJSON *privateAttributeNames;
};


/* Trims a single trailing slash, if present, from the end of the given string.
 * Returns a newly allocated string. */
char *
LDi_trimTrailingSlash(const char *s);

/* Sets target to the trimmed version of s (via LDi_trimTrailingSlash and LDSetString). */
LDBoolean
LDi_setTrimmedString(char **target, const char *s);
