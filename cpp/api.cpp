#include <launchdarkly/api.hpp>

static LDClientCPP theClient;

LDClientCPP *
LDClientCPP::Get()
{
    return &theClient;
}

LDClientCPP *
LDClientCPP::Init(LDConfig *const config, LDUser *const user,
    const unsigned int maxwaitmilli)
{
    theClient.client = LDClientInit(config, user, maxwaitmilli);
    return &theClient;
}

bool
LDClientCPP::isInitialized(void)
{
    return LDClientIsInitialized(this->client);
}

bool
LDClientCPP::awaitInitialized(const unsigned int timeoutmilli)
{
    return LDClientAwaitInitialized(this->client, timeoutmilli);
}

bool
LDClientCPP::boolVariationDetail(const std::string &key,
    const bool def, LDVariationDetails *const details)
{
    return LDBoolVariationDetail(this->client, key.c_str(), def, details);
}

bool
LDClientCPP::boolVariation(const std::string &key, const bool def)
{
    return LDBoolVariation(this->client, key.c_str(), def);
}

int
LDClientCPP::intVariationDetail(const std::string &key,
    const int def, LDVariationDetails *const details)
{
    return LDIntVariationDetail(this->client, key.c_str(), def, details);
}

int
LDClientCPP::intVariation(const std::string &key, const int def)
{
    return LDIntVariation(this->client, key.c_str(), def);
}

double
LDClientCPP::doubleVariationDetail(const std::string &key,
    const double def, LDVariationDetails *const details)
{
    return LDDoubleVariationDetail(this->client, key.c_str(), def, details);
}

double
LDClientCPP::doubleVariation(const std::string &key, const double def)
{
    return LDDoubleVariation(this->client, key.c_str(), def);
}

std::string
LDClientCPP::stringVariationDetail(const std::string &key,
    const std::string &def, LDVariationDetails *const details)
{
    char *const s = LDStringVariationAllocDetail(this->client, key.c_str(),
        def.c_str(), details);
    const std::string res(s);
    LDFree(s);
    return res;
}

std::string
LDClientCPP::stringVariation(const std::string &key, const std::string &def)
{
    char *const s = LDStringVariationAlloc(this->client, key.c_str(),
        def.c_str());
    const std::string res(s);
    LDFree(s);
    return res;
}

char *
LDClientCPP::stringVariationDetail(const std::string &key, const std::string &def,
    char *const buf, const size_t len, LDVariationDetails *const details)
{
    return LDStringVariationDetail(this->client, key.c_str(), def.c_str(), buf,
        len, details);
}

char *
LDClientCPP::stringVariation(const std::string &key,
    const std::string &def, char *const buf, const size_t len)
{
    return LDStringVariation(this->client, key.c_str(), def.c_str(), buf, len);
}

struct LDJSON *
LDClientCPP::JSONVariationDetail(const std::string &key,
    const struct LDJSON *const def, LDVariationDetails *const details)
{
    return LDJSONVariationDetail(this->client, key.c_str(), def, details);
}

struct LDJSON *
LDClientCPP::JSONVariation(const std::string &key,
    const struct LDJSON *const def)
{
    return LDJSONVariation(this->client, key.c_str(), def);
}

struct LDJSON *
LDClientCPP::getAllFlags()
{
    return LDAllFlags(this->client);
}

void
LDClientCPP::flush(void)
{
    LDClientFlush(this->client);
}

void
LDClientCPP::close(void)
{
    LDClientClose(this->client);
    this->client = NULL;
}

void
LDClientCPP::setOffline(void)
{
    LDClientSetOffline(this->client);
}

void
LDClientCPP::setOnline(void)
{
    LDClientSetOnline(this->client);
}

bool
LDClientCPP::isOffline(void)
{
    return LDClientIsOffline(this->client);
}

void
LDClientCPP::setBackground(const bool background)
{
    LDClientSetBackground(this->client, background);
}

void
LDClientCPP::track(const std::string &name)
{
    LDClientTrack(this->client, name.c_str());
}

void
LDClientCPP::track(const std::string &name, struct LDJSON *const data) {
    LDClientTrackData(this->client, name.c_str(), data);
}

void
LDClientCPP::track(const std::string &name, struct LDJSON *const data, double metric) {
    LDClientTrackMetric(this->client, name.c_str(), data, metric);
}

void
LDClientCPP::alias(const struct LDUser *currentUser, const struct LDUser *previousUser) {
    LDClientAlias(this->client, currentUser, previousUser);
}

std::string
LDClientCPP::saveFlags(void)
{
    char *const s = LDClientSaveFlags(this->client);
    std::string rv(s);
    LDFree(s);
    return rv;
}

void
LDClientCPP::restoreFlags(const std::string &flags)
{
    LDClientRestoreFlags(this->client, flags.c_str());
}

void
LDClientCPP::identify(LDUser *const user)
{
    LDClientIdentify(this->client, user);
}

bool
LDClientCPP::registerFeatureFlagListener(const std::string &name,
    LDlistenerfn fn)
{
    return LDClientRegisterFeatureFlagListener(this->client, name.c_str(), fn);
}

void
LDClientCPP::unregisterFeatureFlagListener(const std::string &name,
    LDlistenerfn fn)
{
    LDClientUnregisterFeatureFlagListener(this->client, name.c_str(), fn);
}
