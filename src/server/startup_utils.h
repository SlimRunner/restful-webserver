#ifndef STARTUP_UTILS_H
#define STARTUP_UTILS_H

#include <boost/asio.hpp>
#include <memory>  // shared_ptr
#include <optional>
#include <string>

#include "config_parser.h"
#include "handler_registry.h"

using shared_request_handler = std::shared_ptr<RequestHandler>;

struct config_payload {
    RoutingMap route_map;
    NginxConfig config;
    int port_number;
};

std::optional<std::string> parse_arguments(int, const char* const[]);

std::optional<config_payload> parse_config(std::string filepath);

void init_threads(boost::asio::io_service& io_service, int num_threads);

#endif
