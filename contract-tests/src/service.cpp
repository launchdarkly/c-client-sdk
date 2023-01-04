#include "service/service.hpp"
#include "service/client_entity.hpp"
#include "helpers/http_helpers.hpp"
#include "definitions/definitions.hpp"

#include <iostream>
#include <variant>

namespace ld {

TestService::TestService(std::string name, std::string version) :
        m_name{std::move(name)},
        m_version{std::move(version)},
        m_lock{},
        m_clientCounter{0},
        m_clients{}
{}

TestService::~TestService() = default;

void TestService::get_status(const httplib::Request &req, httplib::Response &res) const {
    nlohmann::json rsp;
    rsp["name"] = m_name;
    rsp["clientVersion"] = m_version;
    rsp["capabilities"] = std::vector<std::string> {
            "client-side",
            "mobile",
            "strongly-typed",
            "service-endpoints",
            "singleton"
    };
    res.set_content(rsp.dump(), "application/json");
}

void TestService::create_client(const httplib::Request &req, httplib::Response &res) {

    auto body = nlohmann::json::parse(req.body);
    auto params = body.get<ld::CreateInstanceParams>();
    auto defaultStartWaitTime = std::chrono::seconds(5);

    std::unique_ptr<ClientEntity> entity = ClientEntity::from(params, defaultStartWaitTime);
    if (!entity) {
        set_server_error(res, "Unable to initialize client");
        return;
    }
    
    std::lock_guard<std::mutex> guard{m_lock};

    m_clientCounter++;
    auto clientID = std::to_string(m_clientCounter);
    m_clients[clientID] = std::move(entity);

    res.set_header("Location", "/clients/" + clientID);
    res.status = 201;
}

void TestService::delete_client(const httplib::Request &req, httplib::Response &res) {

    auto clientID = req.matches[1];

    std::lock_guard<std::mutex> guard{m_lock};

    if (auto iter = m_clients.find(clientID); iter != m_clients.end()) {
        m_clients.erase(iter);
        res.status = 204;
        return;
    }

    set_client_error(res, "client not found");
}

void TestService::do_command(const httplib::Request &req, httplib::Response &res) {

    auto clientID = req.matches[1];
    auto body = nlohmann::json::parse(req.body);
    auto params = body.get<CommandParams>();

    std::lock_guard<std::mutex> guard{m_lock};

    if (auto iter = m_clients.find(clientID); iter != m_clients.end()) {

        JsonOrError out = iter->second->handleCommand(params);
        if (std::holds_alternative<nlohmann::json>(out)) {
            auto json = std::get<nlohmann::json>(out);
            if (!json.is_null()) {
                res.set_content(json.dump(), "application/json");
                res.status = 200;
            } else {
                res.status = 204;
            }
        } else {
            Error error = std::get<Error>(out);
            res.set_content(error.msg, "text/plain");
            res.status = error.code;
        }

        return;
    }

    set_client_error(res, "client not found");
}




} // namespace ld
