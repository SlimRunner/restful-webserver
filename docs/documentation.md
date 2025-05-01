- [Documentation](#documentation)
  - [Project Overview](#project-overview)
  - [Core Components](#core-components)
    - [Request Handling System](#request-handling-system)
      - [`request_handler.h`](#request_handlerh)
      - [`static_file_handler.h`](#static_file_handlerh)
      - [`echo_handler.h`](#echo_handlerh)
    - [Server Infrastructure](#server-infrastructure)
      - [`server.h`](#serverh)
      - [`session.h`](#sessionh)
    - [Configuration](#configuration)
      - [`config_parser.h`](#config_parserh)
    - [Utilities](#utilities)
      - [`logger.h`](#loggerh)
  - [Usage](#usage)


# Documentation

## Project Overview
This project implements a modular HTTP server with customizable request handlers. The server is built using Boost.Asio and follows object-oriented design principles.

## Core Components

### Request Handling System

#### `request_handler.h`
Defines the core interfaces and structures for HTTP request handling:
- `HttpRequest`: Structure representing an HTTP request with method, path, version, headers, and body
- `HttpResponse`: Structure representing an HTTP response with status code, headers, and body
- `RequestHandler`: Abstract base class that all request handlers must implement
- `StatusCode`: Enumeration of HTTP status codes (200 OK, 404 Not Found, etc.)

#### `static_file_handler.h`
Handles requests for static files:
- Serves files from a specified directory
- Maps file extensions to appropriate MIME types
- Performs security checks to prevent directory traversal attacks
- Returns appropriate HTTP errors for invalid requests

#### `echo_handler.h`
Simple handler that echoes back the received request:
- Configured with a path prefix to determine which requests it handles
- Responds with the same content that was sent in the request

### Server Infrastructure

#### `server.h`
Main server class responsible for:
- Accepting incoming TCP connections
- Creating sessions for each client
- Managing request handlers
- Using a session factory pattern for better testability

#### `session.h`
Manages client connections:
- Handles reading data from and writing data to client sockets
- Parses raw HTTP requests into structured format
- Routes requests to appropriate handlers based on path
- Implements safety mechanisms (request size limits, proper cleanup)

### Configuration

#### `config_parser.h`
Parses Nginx-style configuration files:
- Tokenizes and parses configuration syntax
- Supports nested blocks and statements
- Provides structured access to configuration data
- Used to configure server parameters and handlers

### Utilities

#### `logger.h`
Logging utility:
- Initializes the logging system
- Provides consistent logging across the application

## Usage
The server can be configured using an Nginx-style configuration file. Multiple request handlers can be registered, each handling different URL paths.

