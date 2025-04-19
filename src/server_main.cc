//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include "server/server.h"
#include "config/config_parser.h"

using namespace boost::placeholders;

int main(int argc, char* argv[])
{
  if (argc != 2) {
    std::cerr << "Usage: webserver <port>\n";
    return 1;
  }
  // Parse the config file //
  NginxConfigParser config_parser;
  NginxConfig config;
  if (!config_parser.Parse(argv[1], &config)){
    std::cerr << "Failed to parse config file.\n";
    return 1;
  }
  // Find the 'listen' directive
  int port = 0;
  for (const auto& statement : config.statements_) {
    if (statement->tokens_.size() >= 2 && statement->tokens_[0] == "listen") {
      port = std::stoi(statement->tokens_[1]);
      break;
    }
  }
  // If the port could not be found
  if (port == 0) {
    std::cerr << "Error: No valid 'listen <port>;' found in config.\n";
    return 1;
  }

  try {
    boost::asio::io_service io_service;
    //Passing a default session factory which is the same as the original but it can be overidden in testing
    server s(io_service, port, [](auto& io) { return new session(io); });
    io_service.run();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
