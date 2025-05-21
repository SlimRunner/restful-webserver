#include "not_found_handler.h"

#include <array>
#include <memory>

#include "gtest/gtest.h"
#include "handler_registry.h"

// Tests that NotFoundHandler always returns a 404 Not Found response
TEST(NotFoundHandlerTest, AlwaysReturns404) {
    NotFoundHandler nfHandler("/", {});
    HttpRequest req;
    req.path = "/any/path";
    req.version = "HTTP/1.1";
    req.headers = {{"Host", "localhost"}};
    req.body = "some body";

    // Test with different HTTP methods
    const std::array<std::string, 5> methods = {"GET", "POST", "PUT", "DELETE", "PATCH"};
    for (const auto &method : methods) {
        req.method = method;
        std::unique_ptr<HttpResponse> resp = nfHandler.handle_request(req);

        EXPECT_EQ(resp->status, StatusCode::NOT_FOUND);
        EXPECT_EQ(resp->body, "404 Not Found");
        EXPECT_EQ(resp->headers.at("Content-Type"), "text/plain");
        EXPECT_EQ(resp->headers.at("Content-Length"),
                  std::to_string(std::string("404 Not Found").size()));
    }
}

// Tests that the handler can be created via the registry and works correctly
TEST(HandlerRegistryTest, NotFoundHandlerFactoryCreatesWorkingHandler) {
    auto factory = HandlerRegistry::instance().get_factory("NotFoundHandler");
    ASSERT_TRUE(factory != nullptr);

    auto handler = factory("/", {});
    ASSERT_TRUE(handler != nullptr);

    HttpRequest req;
    req.method = "GET";
    req.path = "/unknown";
    req.version = "HTTP/1.1";
    req.headers["Host"] = "localhost";
    req.body = "";

    auto response = handler->handle_request(req);
    ASSERT_TRUE(response != nullptr);
    EXPECT_EQ(response->status, StatusCode::NOT_FOUND);
    EXPECT_EQ(response->body, "404 Not Found");
    EXPECT_EQ(response->headers.at("Content-Type"), "text/plain");
}

// Test scenario where the request comes in with an empty headers or body
// server should still send a 404 rather than crash or misbehave
TEST(NotFoundHandlerTest, HandlesEmptyHeadersAndBody) {
    NotFoundHandler nfHandler("/", {});
    HttpRequest req;
    req.method = "GET";
    req.path = "/nothing";
    req.version = "HTTP/1.1";
    req.headers.clear();  // no headers
    req.body.clear();     // no body

    auto resp = nfHandler.handle_request(req);
    EXPECT_EQ(resp->status, StatusCode::NOT_FOUND);
    EXPECT_EQ(resp->body, "404 Not Found");
    // exactly the two headers we set
    EXPECT_EQ(resp->headers.size(), 2u);
    EXPECT_EQ(resp->headers.at("Content-Type"), "text/plain");
    EXPECT_EQ(resp->headers.at("Content-Length"),
              std::to_string(std::string("404 Not Found").size()));
}

// Test scenario where the request comes in with an empty method or path
// server should still send a 404 rather than crash or misbehave
TEST(NotFoundHandlerTest, HandlesEmptyMethodAndPath) {
    NotFoundHandler nfHandler("/", {});
    HttpRequest req;
    req.method = "";  // missing method
    req.path = "";    // missing path
    req.version = "HTTP/1.1";
    req.headers = {{"Host", "localhost"}};
    req.body = "ignored";

    auto resp = nfHandler.handle_request(req);
    EXPECT_EQ(resp->status, StatusCode::NOT_FOUND);
    EXPECT_EQ(resp->body, "404 Not Found");
    EXPECT_EQ(resp->headers.at("Content-Type"), "text/plain");
    EXPECT_EQ(resp->headers.at("Content-Length"),
              std::to_string(std::string("404 Not Found").size()));
}
