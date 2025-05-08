#ifndef STARTUP_UTILS_H
#define STARTUP_UTILS_H

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../config/config_parser.h"
#include "request_handlers/echo_handler.h"
#include "request_handlers/request_handler.h"
#include "request_handlers/static_file_handler.h"

using shared_request_handler = std::shared_ptr<RequestHandler>;

struct config_payload {
    std::vector<shared_request_handler> handlers;
    NginxConfig config;
    int port_number;
};

std::optional<std::string> parse_arguments(int, const char* const[]);

std::optional<config_payload> parse_config(std::string filepath);

#endif
