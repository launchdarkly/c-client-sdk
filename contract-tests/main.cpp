#include "service/service.hpp"
#include "httplib.h"

#include <launchdarkly/api.h>


const std::unordered_map<const char*, LDLogLevel>& logLevels() {
    static std::unordered_map<const char *, LDLogLevel> levels = {
            {"LD_LOG_TRACE",    LD_LOG_TRACE},
            {"LD_LOG_DEBUG",    LD_LOG_DEBUG},
            {"LD_LOG_INFO",     LD_LOG_INFO},
            {"LD_LOG_WARN",     LD_LOG_WARNING},
            {"LD_LOG_ERROR",    LD_LOG_ERROR},
            {"LD_LOG_CRITICAL", LD_LOG_CRITICAL},
            {"LD_LOG_FATAL",    LD_LOG_FATAL}
    };
    return levels;
}

int main(int argc, char *argv[]) {

    // Unless otherwise specified by the LD_LOG_LEVEL environment variable,
    // use DEBUG as the default level.

    LDLogLevel level = LD_LOG_DEBUG;

    const auto& levels = logLevels();

    if (auto it = levels.find(std::getenv("LD_LOG_LEVEL")); it != levels.end()) {
        level = it->second;
    }

    LDConfigureGlobalLogger(level, LDBasicLogger);

    httplib::Server server{};

    ld::TestService service{
            "c-server-sdk",
            LD_SDK_VERSION,
    };

    server.set_exception_handler([](const auto& req, auto& res, std::exception &e) {
        res.status = 500;
        res.set_content(e.what(), "text/plain");
    });

    server.Get("/", [&](const httplib::Request &req, httplib::Response &res) {
        service.get_status(req, res);
    });

    server.Delete("/", [&](const httplib::Request &, httplib::Response &) {
        server.stop();
    });

    server.Post("/", [&](const httplib::Request &req, httplib::Response &res) {
        service.create_client(req, res);
    });

    server.Delete(R"(/clients/(\d+))", [&](const httplib::Request &req, httplib::Response &res) {
        service.delete_client(req, res);
    });

    server.Post(R"(/clients/(\d+))", [&](const httplib::Request &req, httplib::Response &res) {
        service.do_command(req, res);
    });

    server.listen("localhost", 8000);
    return 0;
}
