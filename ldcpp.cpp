#include "ldapi.h"

#include "ldinternal.h"

static LDClient theClient;

LDClient *LDClient::Get(void)
{
    return &theClient;
}

LDClient *LDClient::Init(LDConfig *const config, LDUser *const user, const unsigned int maxwaitmilli)
{
    theClient.client = LDClientInit(config, user, maxwaitmilli);
    return &theClient;
}

bool LDClient::isInitialized(void)
{
    return LDClientIsInitialized(this->client);
}

bool LDClient::awaitInitialized(const unsigned int timeoutmilli)
{
    return LDClientAwaitInitialized(this->client, timeoutmilli);
}

bool LDClient::boolVariation(const std::string &key, const bool def)
{
	return LDBoolVariation(this->client, key.c_str(), def);
}

int LDClient::intVariation(const std::string &key, const int def)
{
    return LDIntVariation(this->client, key.c_str(), def);
}

double LDClient::doubleVariation(const std::string &key, const double def)
{
    return LDDoubleVariation(this->client, key.c_str(), def);
}

std::string LDClient::stringVariation(const std::string &key, const std::string &def)
{
    char *s = LDStringVariationAlloc(this->client, key.c_str(), def.c_str());
    std::string res(s);
    free(s);
    return res;
}

char * LDClient::stringVariation(const std::string &key, const std::string &def, char *const buf, const size_t len)
{
    return LDStringVariation(this->client, key.c_str(), def.c_str(), buf, len);
}

LDNode *LDClient::JSONVariation(const std::string &key, const LDNode *const def)
{
    return LDJSONVariation(this->client, key.c_str(), def);
}

LDNode *LDClient::getLockedFlags()
{
    return LDClientGetLockedFlags(this->client);
}

LDNode *LDClient::getAllFlags()
{
    return LDAllFlags(this->client);
}

void LDClient::flush(void)
{
    return LDClientFlush(this->client);
}

void LDClient::close(void)
{
    LDClientClose(this->client);
    this->client = 0;
}

LDNode * LDNode::lookup(const std::string &key)
{
    return LDNodeLookup(this, key.c_str());
}

LDNode * LDNode::index(const unsigned int idx)
{
    return LDNodeIndex(this, idx);
}

void LDClient::setOffline(void)
{
    LDClientSetOffline(this->client);
}

void LDClient::setOnline(void)
{
    LDClientSetOnline(this->client);
}

bool LDClient::isOffline(void)
{
    return LDClientIsOffline(this->client);
}

void LDClient::setBackground(const bool background)
{
    LDClientSetBackground(this->client, background);
}

void LDClient::track(const std::string &name)
{
    LDClientTrack(this->client, name.c_str());
}

void LDClient::track(const std::string &name, LDNode *const data)
{
    LDClientTrackData(this->client, name.c_str(), data);
}

std::string LDClient::saveFlags(void)
{
    char *const s = LDClientSaveFlags(this->client);
    std::string rv(s);
    free(s);
    return rv;
}

void LDClient::restoreFlags(const std::string &flags)
{
    LDClientRestoreFlags(this->client, flags.c_str());
}

void LDClient::identify(LDUser *const user)
{
    LDClientIdentify(this->client, user);
}

void LDClient::registerFeatureFlagListener(const std::string &name, LDlistenerfn fn)
{
    LDClientRegisterFeatureFlagListener(this->client, name.c_str(), fn);
}
bool LDClient::unregisterFeatureFlagListener(const std::string &name, LDlistenerfn fn)
{
    return LDClientUnregisterFeatureFlagListener(this->client, name.c_str(), fn);
}
