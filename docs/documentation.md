- [Documentation](#documentation)
  - [Project Overview](#project-overview)
  - [Core Components](#core-components)
    - [Request Handling System](#request-handling-system)
      - [`handler_registry.h`](#handler_registryh)
      - [`request_handler.h`](#request_handlerh)
      - [`static_file_handler.h`](#static_file_handlerh)
      - [`echo_handler.h`](#echo_handlerh)
      - [`not_found_handler.h`](#not_found_handlerh)
    - [Server Infrastructure](#server-infrastructure)
      - [`server.h`](#serverh)
      - [`session.h`](#sessionh)
      - [`startup_utils.h`](#startup_utilsh)
      - [`tagged_exceptions.h`](#tagged_exceptionsh)
    - [Configuration](#configuration)
      - [`config_parser.h`](#config_parserh)
    - [Utilities](#utilities)
      - [`logger.h`](#loggerh)
  - [Usage](#usage)


# Documentation

## Project Overview
This project implements a modular HTTP server with customizable request handlers that are dynamically instantiated using a global registry. The registry is fundamentally a singleton that stores references to factories of handlers see later section to learn how to setup a new handler. The server is built using Boost.Asio and follows object-oriented design principles.

## Core Components

### Request Handling System
Here is a quick overview of mapped prefixes and handler names
| Prefix    | Handler           | Class               |
| --------- | ----------------- | ------------------- |
| `/echo`   | `EchoHandler`     | `EchoHandler`       |
| `/static` | `StaticHandler`   | `StaticFileHandler` |
| `/`       | `NotFoundHandler` | `NotFoundHandler`   |


#### [`handler_registry.h`](/src/server/request_handlers/handler_registry.h)
It systematizes the aggregation of handlers into a singleton that is globally accessible to the project for dynamic instantiation of handlers. It has the following components:
- `RoutingPayload`: stores a handler and argument which servers as a map between handler names and handler custom arguments. It isn't typed to allow flexibility
- `RequestHandlerFactory`: function signature of factories
- `RoutingMap` alias for a map of factory names to their `RoutingPayload`
- `HandlerRegistry`: singleton class that stores and serves the factories.

In order to setup a new factory you have do the following:

* extend the [`request_handler.h`](/src/server/request_handlers/request_handler.h) abstract class
* in your new handler `.cc` file add this
  ```cpp
  #include "my_handler.h"
  // other include directives

  volatile int force_link_my_handler = 0;

  // rest of the handler implementation

  #include "handler_registry.h"
  REGISTER_HANDLER_WITH_NAME(EchoHandler, "EchoHandler")
  ```
* then in [`server_main.cc`](/src/server_main.cc)
  ```cpp
  // include directives

  extern volatile int force_link_my_handler;

  int main(int argc, char *argv[]) {
    (void)force_link_my_handler; // prevents unused warnings
    // rest of main
  }
  ```

#### [`request_handler.h`](/src/server/request_handlers/request_handler.h)
Defines the core interfaces and structures for HTTP request handling:
- `HttpRequest`: Structure representing an HTTP request with method, path, version, headers, and body
- `HttpResponse`: Structure representing an HTTP response with status code, headers, and body
- `RequestHandler`: Abstract base class that all request handlers must implement
- `StatusCode`: Enumeration of HTTP status codes (200 OK, 404 Not Found, etc.)

#### [`static_file_handler.h`](/src/server/request_handlers/static_file_handler.h)
Handles requests for static files:
- Serves files from a specified directory
- Maps file extensions to appropriate MIME types
- Performs security checks to prevent directory traversal attacks
- Returns appropriate HTTP errors for invalid requests

#### [`echo_handler.h`](/src/server/request_handlers/echo_handler.h)
Simple handler that echoes back the received request:
- Configured with a path prefix to determine which requests it handles
- Responds with the same content that was sent in the request

#### [`not_found_handler.h`](/src/server/request_handlers/not_found_handler.h)
Simple handler that returns a 404 for any other path that isn't configured in the project.

### Server Infrastructure

#### [`server.h`](/src/server/server.h)
Main server class responsible for:
- Accepting incoming TCP connections
- Creating sessions for each client
- Managing request handlers
- Using a session factory pattern for better testability

#### [`session.h`](/src/server/session.h)
Manages client connections:
- Handles reading data from and writing data to client sockets
- Parses raw HTTP requests into structured format
- Routes requests to appropriate handlers based on path
- Implements safety mechanisms (request size limits, proper cleanup)

#### [`startup_utils.h`](/src/server/startup_utils.h)
Provides utilities for server startup configuration:
- `config_payload`: Structure containing parsed configuration data including
- `parse_arguments`:
  - it receives `argc` and `argv` from main and returns an empty `optional` if there was an error or the file path of the configuration
- `parse_config`:
  - uses the `config_parser` to load the config into a `config_payload` structure and returns it if it is valid
  - if something fails it returns empty `optional`

#### [`tagged_exceptions.h`](/src/server/tagged_exceptions.h)
Provides an easy to extend base for context rich exceptions. To add one simply add the following to the header file:
```cpp
struct my_custom_exception : public base_tagged_exception {
    using base_tagged_exception::base_tagged_exception;
};
```
Then you can use `my_custom_exception` the same way you would any other exception. It does not allow you carry a custom payload, but that is a trade-off to avoid having you write complex code if all you need is a distinct exception you can catch.

### Configuration

#### [`config_parser.h`](/src/config/config_parser.h)
Parses Nginx-style configuration files:
- Tokenizes and parses configuration syntax
- Supports nested blocks and statements
- Provides structured access to configuration data
- Used to configure server parameters and handlers

### Utilities

#### [`logger.h`](/src/util/logger.h)
Logging utility:
- Initializes the logging system
- Provides consistent logging across the application

## Usage
The server can be configured using an Nginx-style configuration file. Multiple request handlers can be registered, each handling different URL paths. Here is an example log file:
```conf
port 8081;

# Echo handler for /echo path
location /echo EchoHandler { # no arguments
}

# Static file handler for /static path
location /static StaticHandler {
    root ./static;
}
```
