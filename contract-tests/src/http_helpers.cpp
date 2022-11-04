#include "helpers/http_helpers.hpp"

namespace ld {

void set_server_error(httplib::Response &res, const std::string &error) {
    res.status = 500;
    res.set_content(error, "text/plain");
}

void set_client_error(httplib::Response &res, const std::string &error) {
    res.status = 400;
    res.set_content(error, "text/plain");
}

Error make_client_error(std::string msg) {
    return Error{
        400,
        std::move(msg)
    };
}

Error make_server_error(std::string msg) {
    return Error{
        500,
        std::move(msg)
    };
}

} // namespace ld
