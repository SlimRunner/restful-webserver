#include "sleep_handler.h"

#include <chrono>
#include <exception>
#include <sstream>

#include "gtest/gtest.h"
#include "handler_registry.h"

// tests that the handler correctly sleeps for n-seconds
TEST(SleepHandlerTest, TestSleepDuration) {
    SleepHandler sHandler("/sleep", {{"timeout", "200"}});
    HttpRequest req;
    req.path = "/sleep";
    req.version = "HTTP/1.1";
    req.method = "GET";

    auto start = std::chrono::steady_clock::now();
    std::unique_ptr<HttpResponse> resp = sHandler.handle_request(req);
    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // DO NOT check upper bound. Sleep time is non-deterministic because
    // of OS scheduling
    EXPECT_GE(elapsed_ms, 200);
    EXPECT_EQ(resp->status, StatusCode::OK);
}

// tests that the handler correctly rejects any non-GET request
TEST(SleepHandlerTest, RejectsNonGet) {
    SleepHandler sHandler("/sleep", {{"timeout", "0"}});
    HttpRequest req;
    req.path = "/sleep";
    req.version = "HTTP/1.1";

    // NOTE: this list may need modification as the project advances if
    // we implement more HTTP methods.
    const std::array foo = {"get", "POST", "WRONG", "PUT", "PATCH", "DELETE"};

    for (const auto &method : foo) {
        req.method = method;
        std::unique_ptr<HttpResponse> resp = sHandler.handle_request(req);

        EXPECT_EQ(resp->status, StatusCode::BAD_REQUEST);
        EXPECT_EQ(resp->body, "");
        EXPECT_EQ(resp->headers.at("Content-Type"), "text/plain");
        EXPECT_EQ(resp->headers.at("Content-Length"), "0");
    }
}

// tests that the handler correctly rejects any non-GET request
TEST(SleepHandlerTest, RejectsMissingArg) {
    EXPECT_THROW(SleepHandler sHandler("/sleep", {}), std::runtime_error);
}

TEST(SleepHandlerTest, SleepHandlerInRegistry) {
    extern volatile int force_link_sleep_handler;
    (void)force_link_sleep_handler;

    // Assume static dir exists with test.txt file
    auto factory = HandlerRegistry::instance().get_factory("SleepHandler");
    ASSERT_TRUE(factory != nullptr);
}
