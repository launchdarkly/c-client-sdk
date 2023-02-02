#include "service/client_entity.hpp"

namespace ld {

ClientEntity::ClientEntity(struct LDClientCPP *client) : m_client{client} {}

ClientEntity::~ClientEntity() { m_client->close(); }

JsonOrError ClientEntity::handleCommand(const CommandParams &params) {
    switch (params.command) {
        case Command::EvaluateFlag:
            if (!params.evaluate) {
                return make_client_error("Evaluate params should be set");
            }
            return this->evaluate(*params.evaluate);

        case Command::EvaluateAllFlags:
            if (!params.evaluateAll) {
                return make_client_error("EvaluateAll params should be set");
            }
            return this->evaluateAll(*params.evaluateAll);

        case Command::IdentifyEvent:
            if (!params.identifyEvent) {
                return make_client_error("IdentifyEvent params should be set");
            }
            return this->identify(*params.identifyEvent);
        case Command::CustomEvent:
            if (!params.customEvent) {
                return std::string{"CustomEvent params should be set"};
            }
            if (!this->customEvent(*params.customEvent)) {
                return make_server_error("Failed to generate custom event");
            }
            return nlohmann::json{};

        case Command::AliasEvent:
            if (!params.aliasEvent) {
                return make_client_error("AliasEvent params should be set");
            }
            if (!this->aliasEvent(*params.aliasEvent)) {
                return make_server_error("Failed to generate alias event");
            }
            return nlohmann::json{};

        case Command::FlushEvents:
            if (!this->flush()) {
                return make_server_error("Failed to flush events");
            }
            return nlohmann::json{};

        default:
            return make_server_error("Command not supported");
    }
}

JsonOrError ClientEntity::evaluate(const EvaluateFlagParams &params) {

    if (params.detail) {
        return this->evaluateDetail(params);
    }

    const char* key = params.flagKey.c_str();

    auto defaultVal = params.defaultValue;

    EvaluateFlagResponse result;

    switch (params.valueType) {
        case ValueType::Bool:
            result.value = m_client->boolVariation(key,defaultVal.get<LDBoolean>()) != 0;
            break;
        case ValueType::Int:
            result.value = m_client->intVariation(key,defaultVal.get<int>());
            break;
        case ValueType::Double:
            result.value = m_client->doubleVariation(key, defaultVal.get<double>());
            break;
        case ValueType::String: {
            result.value = m_client->stringVariation(key, defaultVal.get<std::string>());
            break;
        }
        case ValueType::Any:
        case ValueType::Unspecified: {
            struct LDJSON* fallback = LDJSONDeserialize(defaultVal.dump().c_str());
            if (!fallback) {
                return make_server_error("JSON appears to be invalid");
            }
            struct LDJSON* evaluation = m_client->JSONVariation(key, fallback);
            LDJSONFree(fallback);

            char *evaluationString = LDJSONSerialize(evaluation);
            LDJSONFree(evaluation);

            if (!evaluationString) {
                return make_server_error("Failed to serialize JSON");
            }
            result.value = nlohmann::json::parse(evaluationString);
            LDFree(evaluationString);
            break;
        }
        default:
            return make_client_error("Unrecognized variation type");

    }

    return result;
}

JsonOrError ClientEntity::evaluateDetail(const EvaluateFlagParams &params) {
    LDVariationDetails details{};

    const char* key = params.flagKey.c_str();

    auto defaultVal = params.defaultValue;

    EvaluateFlagResponse result;

    switch (params.valueType) {
        case ValueType::Bool:
            result.value = m_client->boolVariationDetail(
                    key,
                    defaultVal.get<LDBoolean>(),
                    &details
            ) != 0;
            break;
        case ValueType::Int:
            result.value = m_client->intVariationDetail(
                    key,
                    defaultVal.get<int>(),
                    &details
            );
            break;
        case ValueType::Double:
            result.value = m_client->doubleVariationDetail(
                    key,
                    defaultVal.get<double>(),
                    &details
            );
            break;
        case ValueType::String: {
            result.value = m_client->stringVariationDetail(
                    key,
                    defaultVal.get<std::string>(),
                    &details
            );
            break;
        }
        case ValueType::Any:
        case ValueType::Unspecified: {
            struct LDJSON* fallback = LDJSONDeserialize(defaultVal.dump().c_str());
            if (!fallback) {
                return make_server_error("JSON appears to be invalid");
            }
            struct LDJSON* evaluation = m_client->JSONVariationDetail(
                    key,
                    fallback,
                    &details
            );
            LDJSONFree(fallback);

            char *evaluationString = LDJSONSerialize(evaluation);
            LDJSONFree(evaluation);

            if (!evaluationString) {
                return make_server_error("Failed to serialize JSON");
            }
            result.value = nlohmann::json::parse(evaluationString);
            LDFree(evaluationString);
            break;
        }
        default:
            LDFreeDetailContents(details);
            return make_client_error("Unrecognized variation type");

    }

    result.variationIndex = details.variationIndex >= 0 ? std::optional(details.variationIndex) : std::nullopt;
    result.reason = extract_reason(&details);
    LDFreeDetailContents(details);

    return result;
}


JsonOrError ClientEntity::evaluateAll(const EvaluateAllFlagParams &params) {

    struct LDJSON *json = m_client->getAllFlags();
    if (!json) {
        return make_client_error("unable to retrieve all flags");
    }

    char *jsonStr = LDJSONSerialize(json);
    LDFree(json);

    EvaluateAllFlagsResponse rsp {
        .state = nlohmann::json::parse(jsonStr)
    };

    LDFree(jsonStr);
    return rsp;
}

JsonOrError ClientEntity::identify(const IdentifyEventParams &params) {
    auto user = make_user(params.user);
    if (!user) {
        return make_client_error("ClientEntity::identify: couldn't construct user");
    }
    m_client->identify(user.release());
    /* The SDK test harness relies on the assumption that the identify command is synchronous: after calling it,
     * flag evaluations will take place according to the new user. Since identify() in the C Client is asynchronous,
     * we need to wait for re-initialization. The 1-second timeout is arbitrary. */
    if (!m_client->awaitInitialized(1000)) {
        return make_client_error("ClientEntity::identify: SDK timed out re-initializing after invoking identify()");
    }
    return nlohmann::json{};
}

bool ClientEntity::customEvent(const CustomEventParams &params) {

    struct LDJSON* data = params.data ?
          LDJSONDeserialize(params.data->dump().c_str()) :
          LDNewNull();


    if (params.omitNullData.value_or(false) && !params.metricValue && !params.data) {
        m_client->track(params.eventKey);
        return true;
    }

    if (!params.metricValue) {
        m_client->track(params.eventKey, data);
        return true;
    }

    m_client->track(params.eventKey, data, *params.metricValue);
    return true;
}

bool ClientEntity::aliasEvent(const AliasEventParams &params) {
    auto user = make_user(params.user);
    if (!user) {
        return false;
    }
    auto previousUser = make_user(params.previousUser);
    if (!previousUser) {
        return false;
    }
    m_client->alias(user.get(), previousUser.get());
    return true;
}

bool ClientEntity::flush() {
    m_client->flush();
    return true;
}


std::unique_ptr<ClientEntity> ClientEntity::from(const CreateInstanceParams &params, std::chrono::milliseconds defaultStartWaitTime) {
    struct LDConfig *config = make_config(params.configuration);
    if (!config) {
        return nullptr;
    }

    auto startWaitTime = std::chrono::milliseconds(
            params.configuration.startWaitTimeMs.value_or(defaultStartWaitTime.count())
    );

    auto user = make_user(params.configuration.clientSide->initialUser);

    LDClientCPP *client = LDClientCPP::Init(config, user.release(), startWaitTime.count());

    if (client == nullptr) {
        LDConfigFree(config);
        return nullptr;
    }

    return std::make_unique<ClientEntity>(client);
}

std::unique_ptr<struct LDUser, std::function<void(struct LDUser*)>> make_user(const User &obj) {
    struct LDUser* user = LDUserNew(obj.key.c_str());
    if (!user) {
        return nullptr;
    }
    if (obj.anonymous) {
        LDUserSetAnonymous(user, static_cast<LDBoolean>(*obj.anonymous));
    }
    if (obj.ip) {
        LDUserSetIP(user, obj.ip->c_str());
    }
    if (obj.firstName) {
        LDUserSetFirstName(user, obj.firstName->c_str());
    }
    if (obj.lastName) {
        LDUserSetLastName(user, obj.lastName->c_str());
    }
    if (obj.email) {
        LDUserSetEmail(user, obj.email->c_str());
    }
    if (obj.name) {
        LDUserSetName(user, obj.name->c_str());
    }
    if (obj.avatar) {
        LDUserSetAvatar(user, obj.avatar->c_str());
    }
    if (obj.country) {
        LDUserSetCountry(user, obj.country->c_str());
    }
    if (obj.secondary) {
        LDUserSetSecondary(user, obj.secondary->c_str());
    }
    if (obj.custom) {
        struct LDJSON* json = LDJSONDeserialize(obj.custom->dump().c_str());
        LDUserSetCustom(user, json); // takes ownership of json
    }

    if (obj.privateAttributeNames) {
        for (const std::string& attr: *obj.privateAttributeNames) {
            LDUserAddPrivateAttribute(user, attr.c_str());
        }
    }

    return {user, [](struct LDUser* user){
        LDUserFree(user);
    }};
}

struct LDConfig * make_config(const SDKConfigParams &in) {

    struct LDConfig *out = LDConfigNew(in.credential.c_str());
    if (!out) {
        return nullptr;
    }

    if (in.serviceEndpoints) {
        if (in.serviceEndpoints->streaming) {
            LDConfigSetStreamURI(out, in.serviceEndpoints->streaming->c_str());
        }
        if (in.serviceEndpoints->polling) {
            LDConfigSetAppURI(out, in.serviceEndpoints->polling->c_str());
        }
        if (in.serviceEndpoints->events) {
            LDConfigSetEventsURI(out, in.serviceEndpoints->events->c_str());
        }
    }

    if (in.streaming ) {
        if (in.streaming->baseUri) {
            LDConfigSetStreamURI(out, in.streaming->baseUri->c_str());
        }
    }

    if (in.polling) {
        if (in.polling->baseUri) {
            LDConfigSetAppURI(out, in.polling->baseUri->c_str());
        }
        if (in.polling->pollIntervalMs) {
            LDConfigSetPollingIntervalMillis(out, static_cast<int>(*in.polling->pollIntervalMs));
        }
        if (!in.streaming) {
            LDConfigSetStreaming(out, LDBooleanFalse);
        }
    }


    if (in.events) {
        const SDKConfigEventParams& events = *in.events;

        if (events.baseUri) {
            LDConfigSetEventsURI(out, events.baseUri->c_str());
        }

        if (events.allAttributesPrivate) {
            LDConfigSetAllAttributesPrivate(out, static_cast<LDBoolean>(*events.allAttributesPrivate));
        }

        if (!events.globalPrivateAttributes.empty()) {
            struct LDJSON *privateAttributes = LDNewArray();

            for (const std::string &attr: events.globalPrivateAttributes) {
                LDArrayPush(privateAttributes, LDNewText(attr.c_str()));
            }

            LDConfigSetPrivateAttributes(out, privateAttributes);
        }

        if (events.inlineUsers) {
            LDConfigSetInlineUsersInEvents(out, static_cast<LDBoolean>(*events.inlineUsers));
        }

        if (events.capacity) {
            LDConfigSetEventsCapacity(out, static_cast<int>(*events.capacity));
        }

        if (events.flushIntervalMs) {
            LDConfigSetEventsFlushIntervalMillis(out, *events.flushIntervalMs);
        }
    }

    if (in.clientSide->autoAliasingOptOut) {
        LDConfigAutoAliasOptOut(out, static_cast<LDBoolean>(*in.clientSide->autoAliasingOptOut));
    }

    if (in.clientSide->evaluationReasons) {
        LDConfigSetUseEvaluationReasons(out, static_cast<LDBoolean>(*in.clientSide->evaluationReasons));
    }

    if (in.clientSide->useReport) {
        LDConfigSetUseReport(out, static_cast<LDBoolean>(*in.clientSide->useReport));
    }

    return out;
}


std::optional<nlohmann::json> extract_reason(const LDVariationDetails *details) {
    if (!details->reason) {
        return std::nullopt;
    }
    char* jsonStr = LDJSONSerialize(details->reason);
    nlohmann::json j = nlohmann::json::parse(jsonStr);
    LDFree(jsonStr);
    return std::make_optional(j);
}

} // namespace ld
