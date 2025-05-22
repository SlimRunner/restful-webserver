- [Adding a New Request Handler](#adding-a-new-request-handler)
  - [1. Create the Handler Header](#1-create-the-handler-header)
  - [2. Implement the Handler](#2-implement-the-handler)
  - [3. Force Link in Startup](#3-force-link-in-startup)
  - [4. Update Build Configuration](#4-update-build-configuration)
  - [5. Configure the Handler in Nginx‐Style Config](#5-configure-the-handler-in-nginxstyle-config)
  - [6. Write and Add Tests](#6-write-and-add-tests)

# Adding a New Request Handler

This guide walks you through the steps to add a new HTTP request handler to our server, following the common `RequestHandler` API. We’ll use the [`NotFoundHandler`](../src/server/request_handlers/not_found_handler.{h,cc}) as a running example.

## 1. Create the Handler Header

1. **Location:** `src/server/request_handlers/your_handler.h`
2. **Signature:** Inherit from `RequestHandler`, declare a constructor and override handle_request`.

```cpp
// your_handler.h
#ifndef YOUR_HANDLER_H
#define YOUR_HANDLER_H

#include "request_handler.h"

// Brief description of what this handler does
class YourHandler : public RequestHandler {
 public:
  // Constructs a handler for requests matching `path_prefix`
  YourHandler(const std::string& path_prefix,
              const std::map<std::string, std::string>& args);

  // Handles the HTTP request, returning a HttpResponse pointer
  std::unique_ptr<HttpResponse> handle_request(const HttpRequest& request) override;

 private:
  std::string path_prefix_;  // URL prefix this handler matches
};

#endif  // YOUR_HANDLER_H
```

> **Example:** In `not_found_handler.h`, we declare `NotFoundHandler` exactly this way, with a constructor taking `(path_prefix, args)` and an override for `handle_request` that always returns a 404.

## 2. Implement the Handler

1. **Location:** `src/server/request_handlers/your_handler.cc`
2. **Include:** Your header, any logging or utilities.
3. **Linkage:** Define a `volatile int force_link_*` to force the linker to include this translation unit.
4. **Constructor:** Initialize `path_prefix_` from the parameter.
5. **`handle_request`:** Build and return a `HttpResponse`.
6. **Registration:** Use `REGISTER_HANDLER_WITH_NAME` to register under a config name.

```cpp
// your_handler.cc
#include "your_handler.h"
#include <boost/log/trivial.hpp>

// Ensure this file is linked
volatile int force_link_your_handler = 0;

/*
 Constructs a YourHandler for the given path prefix.
 - path_prefix: URL prefix (e.g. "/foo")
 - args: config parameters (e.g. root paths, options)
*/
YourHandler::YourHandler(
    const std::string& path_prefix,
    const std::map<std::string, std::string>& args)
    : path_prefix_(path_prefix) {
  // Optionally parse args["key"] here
}

/*
 Handles incoming requests matching the prefix.
*/
std::unique_ptr<HttpResponse> YourHandler::handle_request(
    const HttpRequest& request) {
  BOOST_LOG_TRIVIAL(info)
      << "YourHandler: handling " << request.method << " " << request.path;

  HttpResponse response;
  // Set status, headers, body
  response.status = StatusCode::OK;
  response.headers["Content-Type"] = "text/plain";
  response.body = "Hello from YourHandler!";
  response.headers["Content-Length"] = std::to_string(response.body.size());

  return std::make_unique<HttpResponse>(response);
}

// Register under the name used in configs
#include "handler_registry.h"
REGISTER_HANDLER_WITH_NAME(YourHandler, "YourHandler")
```

> **Example:** `not_found_handler.cc` follows this pattern to return a 404 response and logs the unmatched path.

## 3. Force Link in Startup

In `src/server/main_server.cc`, declare and reference your `force_link_*` symbol so the build and tests link your handler:

```diff
 extern volatile int force_link_echo_handler;
 extern volatile int force_link_static_handler;
+extern volatile int force_link_your_handler;

 void InitAllHandlers() {
   (void)force_link_echo_handler;
   (void)force_link_static_handler;
+  (void)force_link_your_handler;
   // ...
 }
```

> **Example:** We added `force_link_not_found_handler` and invoked it in `main_server.cc`.

## 4. Update Build Configuration

Add your new source file(s) to the appropriate targets in `CMakeLists.txt`:

```diff
 add_library(request_handlers
     src/server/request_handlers/echo_handler.cc
     src/server/request_handlers/static_file_handler.cc
+    src/server/request_handlers/your_handler.cc
     handler_registry.cc
 )

 add_executable(webserver
     ...
 )
```

And for tests:

```diff
 add_executable(request_handlers_test
     tests/server/request_handlers/echo_handler_test.cc
     tests/server/request_handlers/static_file_handler_test.cc
+    tests/server/request_handlers/your_handler_test.cc
 )
```

> **Example:** We inserted `not_found_handler.cc` and `not_found_handler_test.cc` into the build.

## 5. Configure the Handler in Nginx‐Style Config

In `configs/server_config.txt` (and `cloud_config.txt`), add a location block using your handler’s registered name:

```nginx
location /foo YourHandler {
    # optional args: key value pairs, e.g. root /data;
}
```

> **Example:** We added:
>
> ```nginx
> # Catch all unmatched paths
> location / NotFoundHandler { }
> ```

## 6. Write and Add Tests

* Create a `your_handler_test.cc` under `tests/server/request_handlers/`.
* Mirror existing patterns:

  * Test responses for valid and invalid methods
  * Verify headers, status codes, and body content
  * Include registry‐instantiation tests
  * Add edge cases (empty headers, missing path, etc.)

> **Example:** `not_found_handler_test.cc` covers status, body, headers, and edge inputs.

With these steps, you can add, build, configure, and test any new request handler consistently following the project conventions. Feel free to reference the `EchoHandler` and `NotFoundHandler` implementations as templates!
