#include "echo_handler.h"

#include <boost/log/trivial.hpp>
#include <sstream>

/*
Constructs an EchoHandler for requests matching the given path prefix.
- path_prefix: the URL prefix this handler will respond to (e.g. "/echo").
*/
EchoHandler::EchoHandler(const std::string &path_prefix) : path_prefix_(path_prefix) {}

/*
Handles an HTTP GET request by echoing the full request back in the response body.
Returns 400 Bad Request if the method is not GET.
*/
HttpResponse EchoHandler::HandleRequest(const HttpRequest &request) {
    HttpResponse response;

    // Check if the request method is valid (GET)
    if (request.method != "GET") {
        BOOST_LOG_TRIVIAL(warning) << "EchoHandler rejected non-GET request: " << request.method;
        response.status = StatusCode::BAD_REQUEST;
        response.body = "";
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = "0";
        return response;
    }
    BOOST_LOG_TRIVIAL(info) << "EchoHandler handling GET request for path: " << request.path;

    response.status = StatusCode::OK;

    // Reconstruct the full request for echoing
    std::string full_request = request.method + " " + request.path + " " + request.version + "\r\n";
    for (const auto &header : request.headers) {
        full_request += header.first + ": " + header.second + "\r\n";
    }
    full_request += "\r\n" + request.body;

    response.body = full_request;
    response.headers["Content-Type"] = "text/plain";
    response.headers["Content-Length"] = std::to_string(response.body.size());

    return response;
}

/*
Checks whether this handler should handle the given request path.
Returns true if the path starts with the handler’s path prefix.
*/
bool EchoHandler::CanHandle(const std::string &path) const {
    return path.substr(0, path_prefix_.size()) == path_prefix_;
}
