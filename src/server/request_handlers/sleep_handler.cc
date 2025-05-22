#include "sleep_handler.h"

#include <boost/log/trivial.hpp>
#include <chrono>
#include <sstream>
#include <thread>

volatile int force_link_sleep_handler = 0;

/*
Constructs an SleepHandler for requests matching the given path prefix.
- path_prefix: the URL prefix this handler will respond to (e.g. "/echo").
- map essentially takes all the rest of the configs (e.g. "root: /file")
  this then puts it into a map for fast access
*/
SleepHandler::SleepHandler(const std::string &path_prefix,
                           const std::map<std::string, std::string> &args)
    : path_prefix_(path_prefix) {
    auto it = args.find("timeout");
    if (it != args.end()) {
        std::stringstream num(it->second);
        // WARNING: this handler is for testing ONLY. Add a try catch
        // here if you ever re-purpose it for public facing server, or
        // throw a meaningful exception. See tagged_exceptions.
        num >> delay_in_millis_;
    } else {
        throw std::runtime_error("SleepHandler requires a 'timeout' argument.");
    }
}

/*
Handles an HTTP GET request by sleeping for the specified amount in config.
Returns 400 Bad Request if the method is not GET.
*/
std::unique_ptr<HttpResponse> SleepHandler::handle_request(const HttpRequest &request) {
    HttpResponse response = {};

    // Check if the request method is valid (GET)
    if (request.method != "GET") {
        BOOST_LOG_TRIVIAL(warning) << "SleepHandler rejected non-GET request: " << request.method;
        response.status = StatusCode::BAD_REQUEST;
        response.body = "";
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = "0";
        return std::make_unique<HttpResponse>(response);
    }
    BOOST_LOG_TRIVIAL(info) << "SleepHandler handling GET request to sleep";

    response.status = StatusCode::OK;
    std::chrono::milliseconds time_in_ms(delay_in_millis_);
    BOOST_LOG_TRIVIAL(info) << "sleep for " << delay_in_millis_ << " ms...";

    std::this_thread::sleep_for(time_in_ms);
    BOOST_LOG_TRIVIAL(info) << "sleep completed";

    std::ostringstream request_body;
    request_body << "Slept for " << delay_in_millis_ << "milliseconds" << "\r\n";

    response.body = request_body.str();
    response.headers["Content-Type"] = "text/plain";
    response.headers["Content-Length"] = std::to_string(response.body.size());

    return std::make_unique<HttpResponse>(response);
}

/*
This program is ran at startup, before main()
It lets the registry know that if we need SleepHandler it will start building it
*/
#include "handler_registry.h"
REGISTER_HANDLER_WITH_NAME(SleepHandler, "SleepHandler")
