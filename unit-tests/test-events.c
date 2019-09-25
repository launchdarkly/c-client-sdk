#include "ldapi.h"
#include "ldinternal.h"

LDClient *
makeTestClient()
{
    LDConfig *config;
    LDUser *user;
    LDClient *client;

    LD_ASSERT(config = LDConfigNew("abc"));
    LDConfigSetOffline(config, true);

    LD_ASSERT(user = LDUserNew("test-user"));

    LD_ASSERT(client = LDClientInit(config, user, 0));

    return client;
}

void
testTrackMetricQueued()
{
    double metricValue;
    const char *metricName;
    LDClient *client;
    cJSON *event;

    metricValue = 0.52;
    metricName = "metricName";
    LD_ASSERT(client = makeTestClient());

    LDClientTrackMetric(client, metricName, NULL, metricValue);

    LD_ASSERT(cJSON_GetArraySize(client->eventArray) == 2);
    LD_ASSERT(event = cJSON_GetArrayItem(client->eventArray, 1));
    LD_ASSERT(metricValue == cJSON_GetObjectItemCaseSensitive(
        event, "metricValue")->valuedouble);
    LD_ASSERT(strcmp(metricName, cJSON_GetObjectItemCaseSensitive(
        event, "key")->valuestring) == 0);
    LD_ASSERT(strcmp("custom", cJSON_GetObjectItemCaseSensitive(
        event, "kind")->valuestring) == 0);

    LDClientClose(client);
}

int
main()
{
    testTrackMetricQueued();

    return 0;
}
