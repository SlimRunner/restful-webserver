#include "echo_handler.h"

#include <array>
#include <memory>
#include <sstream>

#include "gtest/gtest.h"
#include "handler_registry.h"

// tests that the handler correctly generates a GET response
TEST(EchoHandlerTest, HandleGetRequestEchoesRequest) {
    EchoHandler eHandler("/echo", {});
    HttpRequest req;
    req.method = "GET";
    req.path = "/echo/path";
    req.version = "HTTP/1.1";
    req.headers = {{"Host", "localhost"}, {"Accept", "*/*"}};
    req.body = "test body";

    std::shared_ptr<HttpResponse> resp = eHandler.handle_request(req);

    EXPECT_EQ(resp->status, StatusCode::OK);

    std::ostringstream oss;

    // reconstruct headers mimicking internal behavior
    oss << req.method << " " << req.path << " " << req.version << "\r\n";
    for (auto &hdr : req.headers) {
        oss << hdr.first << ": " << hdr.second << "\r\n";
    }
    oss << "\r\n" << req.body;
    const auto expected = oss.str();

    EXPECT_EQ(resp->body, expected);
    EXPECT_EQ(resp->headers.at("Content-Type"), "text/plain");
    EXPECT_EQ(resp->headers.at("Content-Length"), std::to_string(expected.size()));
}

// tests that the handler correctly rejects any non-GET request
TEST(EchoHandlerTest, RejectsNonGet) {
    EchoHandler eHandler("/echo", {});
    HttpRequest req;
    req.path = "/echo";
    req.version = "HTTP/1.1";
    req.headers = {{"Content-Type", "text/plain"}};
    req.body = "body";

    // NOTE: this list may need modification as the project advances if
    // we implement more HTTP methods.
    const std::array foo = {"get", "POST", "WRONG", "PUT", "PATCH", "DELETE"};

    for (const auto &method : foo) {
        req.method = method;
        std::shared_ptr<HttpResponse> resp = eHandler.handle_request(req);

        EXPECT_EQ(resp->status, StatusCode::BAD_REQUEST);
        EXPECT_EQ(resp->body, "");
        EXPECT_EQ(resp->headers.at("Content-Type"), "text/plain");
        EXPECT_EQ(resp->headers.at("Content-Length"), "0");
    }
}

TEST(HandlerRegistryTest, EchoHandlerFactoryCreatesWorkingHandler) {
    auto factory = HandlerRegistry::instance().get_factory("EchoHandler");
    ASSERT_TRUE(factory != nullptr);

    auto handler = factory("/echo", {});
    ASSERT_TRUE(handler != nullptr);

    HttpRequest req;
    req.method = "GET";
    req.path = "/echo";
    req.version = "HTTP/1.1";
    req.headers["Host"] = "localhost";
    req.body = "Hello world";

    auto response = handler->handle_request(req);
    ASSERT_TRUE(response != nullptr);
    EXPECT_EQ(response->status, StatusCode::OK);
    EXPECT_TRUE(response->body.find("GET /echo HTTP/1.1") != std::string::npos);
    EXPECT_TRUE(response->body.find("Hello world") != std::string::npos);
}
