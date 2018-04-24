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

void LDClient::flush(void)
{
    return LDClientFlush(this->client);
}

void LDClient::close(void)
{
    LDClientClose(this->client);
    this->client = 0;
}