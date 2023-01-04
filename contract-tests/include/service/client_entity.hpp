#pragma once

#include "definitions/definitions.hpp"
#include "helpers/http_helpers.hpp"

#include <nlohmann/json.hpp>
#include <launchdarkly/api.hpp>

#include <functional>
#include <memory>
#include <chrono>
#include <variant>

namespace ld {


using UserDeleter = std::function<void(struct LDUser*)>;
using UserPtr = std::unique_ptr<struct LDUser, UserDeleter>;
// Creates an LDUser with automatic destruction.
UserPtr make_user(const User &name);


// Creates an LDConfig from the given request parameters.
// Cannot wrap LDConfig in a std::unique_ptr, because the semantics don't properly convey its interaction
// with LDClient. This is because ownership is transferred to the LDClient constructor - but _only_ if the
// constructor succeeds. If the client cannot be initialized, then the caller must free the LDConfig manually.
// Calling .release() on the pointer would make sense if LDClient took ownership always, but it doesn't.
struct LDConfig * make_config(const SDKConfigParams &in);

// Extracts a JSON representation of an evaluation reason, if it exists.
std::optional<nlohmann::json> extract_reason(const LDVariationDetails *details);

using JsonOrError = std::variant<nlohmann::json, Error>;

class ClientEntity {
public:
    explicit ClientEntity(struct LDClientCPP *);

    ~ClientEntity();

    JsonOrError handleCommand(const CommandParams &params);

    JsonOrError evaluate(const EvaluateFlagParams &params);

    JsonOrError evaluateDetail(const EvaluateFlagParams &params);

    JsonOrError evaluateAll(const EvaluateAllFlagParams &params);

    bool identify(const IdentifyEventParams &params);

    bool customEvent(const CustomEventParams &params);

    bool aliasEvent(const AliasEventParams &params);

    bool flush();

    static std::unique_ptr <ClientEntity>
    from(const CreateInstanceParams &params, std::chrono::milliseconds defaultStartWaitTime);


private:
    struct LDClientCPP *m_client;
};

} // namespace ld
