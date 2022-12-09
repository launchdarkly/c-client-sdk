#pragma once

#include <httplib.h>

#include <string>
#include <memory>
#include <unordered_map>

namespace ld {

class ClientEntity;

class TestService {
public:
    explicit TestService(std::string name, std::string version);
    ~TestService();

    void get_status(const httplib::Request &req, httplib::Response &res) const;

    void create_client(const httplib::Request &req, httplib::Response &res);

    void delete_client(const httplib::Request &req, httplib::Response &res);

    void do_command(const httplib::Request &req, httplib::Response &res);
private:
    std::string m_name;
    std::string m_version;
    std::mutex m_lock;
    std::uint32_t m_clientCounter;
    std::unordered_map<std::string, std::unique_ptr<ClientEntity>> m_clients;
};

} // namespace ld
