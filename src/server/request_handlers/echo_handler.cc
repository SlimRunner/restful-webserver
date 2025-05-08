#include "echo_handler.h"

#include <boost/log/trivial.hpp>
#include <sstream>

volatile int force_link_echo_handler = 0;


/*
Constructs an EchoHandler for requests matching the given path prefix.
- path_prefix: the URL prefix this handler will respond to (e.g. "/echo").
- map essentially takes all the rest of the configs (e.g. "root: /file")
  this then puts it into a map for fast access
*/
EchoHandler::EchoHandler(const std::string &path_prefix, 
                         const std::map<std::string, std::string>& /*args*/) 
    : path_prefix_(path_prefix) {}

/*
Handles an HTTP GET request by echoing the full request back in the response body.
Returns 400 Bad Request if the method is not GET.
*/
std::shared_ptr<HttpResponse> EchoHandler::handle_request(const HttpRequest &request) {
    auto response = std::make_shared<HttpResponse>();

    // Check if the request method is valid (GET)
    if (request.method != "GET") {
        BOOST_LOG_TRIVIAL(warning) << "EchoHandler rejected non-GET request: " << request.method;
        response->status = StatusCode::BAD_REQUEST;
        response->body = "";
        response->headers["Content-Type"] = "text/plain";
        response->headers["Content-Length"] = "0";
        return response;
    }
    BOOST_LOG_TRIVIAL(info) << "EchoHandler handling GET request for path: " << request.path;

    response->status = StatusCode::OK;

    // Reconstruct the full request for echoing
    std::string full_request = request.method + " " + request.path + " " + request.version + "\r\n";
    for (const auto &header : request.headers) {
        full_request += header.first + ": " + header.second + "\r\n";
    }
    full_request += "\r\n" + request.body;

    response->body = full_request;
    response->headers["Content-Type"] = "text/plain";
    response->headers["Content-Length"] = std::to_string(response->body.size());

    return response;
}

/*
This program is ran at startup, before main()
It lets the registry know that if we need EchoHandler it will start building it 
*/
#include "handler_registry.h"
REGISTER_HANDLER_WITH_NAME(EchoHandler, "EchoHandler")
