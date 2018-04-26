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

void LDClient::flush(void)
{
    return LDClientFlush(this->client);
}

void LDClient::close(void)
{
    LDClientClose(this->client);
    this->client = 0;
}