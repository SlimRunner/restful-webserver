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
#include <memory>
#include <vector>
#include "server/server.h"
#include "config/config_parser.h"
#include "server/request_handlers/request_handler.h"
#include "server/request_handlers/static_file_handler.h"
#include "server/request_handlers/echo_handler.h"
#include "logger.h"
#include <boost/log/trivial.hpp>

using namespace boost::placeholders;

int main(int argc, char *argv[])
{
  // Initialize logging
  init_logging();
  BOOST_LOG_TRIVIAL(info) << "Server starting...";
  if (argc != 2)
  {
    std::cerr << "Usage: webserver <port>\n";
    return 1;
  }

  // Parse the config file //
  NginxConfigParser config_parser;
  NginxConfig config;
  if (!config_parser.Parse(argv[1], &config))
  {
    std::cerr << "Failed to parse config file.\n";
    return 1;
  }

  // Find the 'listen' directive
  int port = 0;
  for (const auto &statement : config.statements_)
  {
    if (statement->tokens_.size() >= 2 && statement->tokens_[0] == "listen")
    {
      port = std::stoi(statement->tokens_[1]);
      break;
    }
  }

  // If the port could not be found
  if (port == 0)
  {
    std::cerr << "Error: No valid 'listen <port>;' found in config.\n";
    return 1;
  }

  // Create handlers based on the config
  std::vector<std::shared_ptr<RequestHandler>> handlers;

  for (const auto &statement : config.statements_)
  {
    // Path configuration blocks start with a path (e.g., "/static", /echo)
    if (statement->tokens_.size() > 0 && statement->tokens_[0][0] == '/' && statement->child_block_)
    {
      std::string path_prefix = statement->tokens_[0];
      std::string handler_type;
      std::string root_dir;

      // Look for handleer type and root directory in the child block
      for (const auto &child_statement : statement->child_block_->statements_)
      {
        if (child_statement->tokens_.size() >= 2)
        {
          if (child_statement->tokens_[0] == "handler")
          {
            handler_type = child_statement->tokens_[1];
          }
          else if (child_statement->tokens_[0] == "root")
          {
            root_dir = child_statement->tokens_[1];
          }
        }
      }

      // Create the appropriate handler based on the type
      if (handler_type == "EchoHandler")
      {
        handlers.push_back(std::make_shared<EchoHandler>(path_prefix));
      }
      else if (handler_type == "StaticHandler" && !root_dir.empty())
      {
        handlers.push_back(std::make_shared<StaticFileHandler>(path_prefix, root_dir));
      }
      else
      {
        std::cerr << "Unknown handler type: " << handler_type << "\n";
      }
    }
  }

  // If no handlers were configured, default to EchoHandler
  if (handlers.empty())
  {
    handlers.push_back(std::make_shared<EchoHandler>("/"));
    std::cout << "No handlers configured. Defaulting to EchoHandler.\n";
  }

  try
  {
    boost::asio::io_service io_service;

    // Create the server with the configured handlers
    server s(io_service, port, handlers,
             [](auto &io, auto handlers)
             { return new session(io, handlers); });
    std::cout << "Server started on port " << port << "\n";
    io_service.run();
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
