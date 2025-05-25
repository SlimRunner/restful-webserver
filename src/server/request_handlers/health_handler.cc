#include "health_handler.h"

#include <boost/log/trivial.hpp>

// to ensure this translation unit gets linked in
volatile int force_link_health_handler = 0;

/*
 Constructs a HealthHandler for requests matching the given path prefix.
 - path_prefix: the URL prefix this handler will respond to (e.g. "/health")
 - args: map of additional config parameters (unused here)
*/
HealthHandler::HealthHandler(const std::string& path_prefix,
                             const std::map<std::string, std::string>& /*args*/)
    : path_prefix_(path_prefix) {}

/*
 Handles any HTTP request by returning a 200 OK response with body "OK".
 Used for health checks and uptime monitoring systems.
*/
std::unique_ptr<HttpResponse> HealthHandler::handle_request(const HttpRequest& request) {
    BOOST_LOG_TRIVIAL(info) << "HealthHandler: Received " << request.method
                            << " request to " << request.path;

    HttpResponse response = {};
    response.status = StatusCode::OK;
    std::string body = "OK";
    response.body = body;
    response.headers["Content-Type"] = "text/plain";
    response.headers["Content-Length"] = std::to_string(body.size());
    return std::make_unique<HttpResponse>(response);
}

/*
This program is run at startup, before main().
It registers the HealthHandler in the handler registry so it can be instantiated by name.
Registered under the name used in the config files.
*/
#include "handler_registry.h"
REGISTER_HANDLER_WITH_NAME(HealthHandler, "HealthHandler")
