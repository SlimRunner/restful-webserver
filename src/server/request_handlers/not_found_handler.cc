#include "not_found_handler.h"

#include <boost/log/trivial.hpp>

// to ensure this translation unit gets linked in
volatile int force_link_not_found_handler = 0;

/*
 Constructs a NotFoundHandler for requests matching the given path prefix.
 - path_prefix: the URL prefix this handler will respond to (e.g. "/")
 - args: map of additional config parameters (unused here)
*/
NotFoundHandler::NotFoundHandler(const std::string& path_prefix,
                                 const std::map<std::string, std::string>& /*args*/)
    : path_prefix_(path_prefix) {}

/*
 Handles any HTTP request by returning a 404 Not Found response.
 Always logs the unmatched path and sets the appropriate status, headers, and body.
*/
std::unique_ptr<HttpResponse> NotFoundHandler::handle_request(const HttpRequest& request) {
    BOOST_LOG_TRIVIAL(info) << "NotFoundHandler: Triggered by no matching handler for path "
                            << request.path;

    HttpResponse response = {};
    response.status = StatusCode::NOT_FOUND;
    std::string body = "404 Not Found";
    response.body = body;
    response.headers["Content-Type"] = "text/plain";
    response.headers["Content-Length"] = std::to_string(body.size());
    return std::make_unique<HttpResponse>(response);
}
/*
This program is ran at startup, before main()
It lets the registry know that if we need NotFoundHandler it will start building it
Registered under the name used in the config files
*/
#include "handler_registry.h"
REGISTER_HANDLER_WITH_NAME(NotFoundHandler, "NotFoundHandler")
