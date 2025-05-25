#include "health_handler.h"
#include "gtest/gtest.h"
#include "handler_registry.h"

// Test that HealthHandler returns 200 OK with the expected headers and body
TEST(HealthHandlerTest, Returns200OK) {
    HealthHandler handler("/health", {});
    HttpRequest req;
    req.method = "GET";
    req.path = "/health";
    req.version = "HTTP/1.1";

    auto response = handler.handle_request(req);

    ASSERT_EQ(response->status, StatusCode::OK);
    EXPECT_EQ(response->body, "OK");
    EXPECT_EQ(response->headers.at("Content-Type"), "text/plain");
    EXPECT_EQ(response->headers.at("Content-Length"), std::to_string(response->body.size()));
}

// Test that HealthHandler can be constructed dynamically using the handler registry
TEST(HealthHandlerTest, CanBeInstantiatedFromRegistry) {
    auto& registry = HandlerRegistry::instance();

    // Attempt to retrieve the factory associated with "HealthHandler"
    auto factory = registry.get_factory("HealthHandler");

    // Factory should return a valid handler instance
    std::shared_ptr<RequestHandler> handler = factory("/health", {});
    ASSERT_NE(handler, nullptr);
}

// Test that HealthHandler does not break on unexpected HTTP methods (e.g., POST)
TEST(HealthHandlerTest, IgnoresMethodAndStillReturnsOK) {
    HealthHandler handler("/health", {});
    HttpRequest req;
    req.method = "POST";  // Unexpected method, but handler should still return 200 OK
    req.path = "/health";
    req.version = "HTTP/1.1";

    auto response = handler.handle_request(req);
    ASSERT_EQ(response->status, StatusCode::OK);
    EXPECT_EQ(response->body, "OK");
}
