//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/log/trivial.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "logger.h"
#include "server/server.h"
#include "server/startup_utils.h"
#include "server/tagged_exceptions.h"

using namespace boost::placeholders;
extern volatile int force_link_echo_handler;
extern volatile int force_link_static_handler;

int main(int argc, char *argv[]) {
    (void)force_link_echo_handler;
    (void)force_link_static_handler;
    try {
        init_logging();

        BOOST_LOG_TRIVIAL(info) << "Server starting...";
        auto arguments = parse_arguments(argc, argv);

        if (!arguments.has_value()) {
            throw std::invalid_argument("");
        }

        std::string config_file_path = arguments.value();
        BOOST_LOG_TRIVIAL(info) << "argument received: " << config_file_path;

        if (!std::filesystem::exists(config_file_path)) {
            throw std::invalid_argument("Provided config file does not exist");
        }

        auto config_result = parse_config(config_file_path);

        if (!config_result.has_value()) {
            throw expt::config_exception("Config parsing failed.");
        }

        const config_payload &config = config_result.value();
        BOOST_LOG_TRIVIAL(info) << "Config loaded successfully.";

        boost::asio::io_service io_service;

        // Create the server with the configured handlers
        server s(io_service, config.port_number, config.handlers,
                 [](auto &io, auto handlers) { return new session(io, handlers); });
        BOOST_LOG_TRIVIAL(info) << "Server started successfully on port " << config.port_number;
        io_service.run();

    } catch (std::invalid_argument &e) {
        BOOST_LOG_TRIVIAL(error) << "Invalid argument: " << e.what();
        BOOST_LOG_TRIVIAL(error) << "Server failed to start.";
        return 1;

    } catch (expt::config_exception &e) {
        BOOST_LOG_TRIVIAL(error) << "Invalid configuration: " << e.what();
        BOOST_LOG_TRIVIAL(error) << "Server failed to start.";
        return 1;

    } catch (std::exception &e) {
        BOOST_LOG_TRIVIAL(error) << "Exception during server runtime: " << e.what();
        BOOST_LOG_TRIVIAL(error) << "Server shutting down due to exception.";
        return 1;
    }

    BOOST_LOG_TRIVIAL(info) << "Server exited cleanly.";
    return 0;
}
