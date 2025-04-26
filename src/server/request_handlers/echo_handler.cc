#include "echo_handler.h"

#include <boost/log/trivial.hpp>
#include <sstream>

EchoHandler::EchoHandler(const std::string &path_prefix) : path_prefix_(path_prefix) {}

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

bool EchoHandler::CanHandle(const std::string &path) const {
    return path.substr(0, path_prefix_.size()) == path_prefix_;
}
