#include "ldapi.h"

#include "ldinternal.h"

static LDClient theClient;

LDClient *LDClient::Get(void)
{
    return &theClient;
}

LDClient *LDClient::Init(LDConfig *config, LDUser *user)
{
    theClient.client = LDClientInit(config, user);
    return &theClient;
}

bool LDClient::isInitialized(void)
{
    return LDClientIsInitialized(this->client);
}

bool LDClient::boolVariation(const std::string &key, bool def)
{
	return LDBoolVariation(this->client, key.c_str(), def);
}

int LDClient::intVariation(const std::string &key, int def)
{
    return LDIntVariation(this->client, key.c_str(), def);
}

double LDClient::doubleVariation(const std::string &key, double def)
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

char * LDClient::stringVariation(const std::string &key, const std::string &def, char *buf, size_t len)
{
    return LDStringVariation(this->client, key.c_str(), def.c_str(), buf, len);
}

LDMapNode *LDClient::JSONVariation(const std::string &key, LDMapNode *def)
{
    return LDJSONVariation(this->client, key.c_str(), def);
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

LDMapNode * LDMapNode::lookup(const std::string &key)
{
    return LDMapLookup(this, key.c_str());
}

void LDMapNode::release(void)
{
    LDJSONRelease(this);
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

std::string LDClient::saveFlags(void)
{
    char *s = LDClientSaveFlags(this->client);
    std::string rv(s);
    free(s);
    return rv;
}

void LDClient::restoreFlags(const std::string &flags)
{
    LDClientRestoreFlags(this->client, flags.c_str());
}