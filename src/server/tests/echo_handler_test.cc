#include "../request_handlers/echo_handler.h"
#include "gtest/gtest.h"
#include <array>
#include <sstream>

// tests that the handler can handle valid paths of different lengths
TEST(EchoHandlerTest, CanHandleGoodPrefixes) {
  EchoHandler eHandler("/echo");
  // assumptions: correct handlers...
  // - don't ignore slashes
  // - allow any level of depth
  // - are case-sensitive
  EXPECT_TRUE(eHandler.CanHandle("/echo"));
  EXPECT_TRUE(eHandler.CanHandle("/echo/hello"));
  EXPECT_TRUE(eHandler.CanHandle("/echo/hello/world/file"));
}

// tests that the handler correctly rejects non-matching paths
TEST(EchoHandlerTest, CanHandleBadPrefixes) {
  EchoHandler eHandler("/echo");
  // assumptions: correct handlers...
  // - don't ignore slashes
  // - allow any level of depth
  // - are case-sensitive
  EXPECT_FALSE(eHandler.CanHandle("echo"));
  EXPECT_FALSE(eHandler.CanHandle("/ech"));
  EXPECT_FALSE(eHandler.CanHandle("/ECHO"));
  EXPECT_FALSE(eHandler.CanHandle("/other"));
}

// tests that the handler correctly generates a GET response
TEST(EchoHandlerTest, HandleGetRequestEchoesRequest) {
  EchoHandler eHandler("/echo");
  HttpRequest req;
  req.method = "GET";
  req.path = "/echo/path";
  req.version = "HTTP/1.1";
  req.headers = {{"Host", "localhost"}, {"Accept", "*/*"}};
  req.body = "test body";

  HttpResponse resp = eHandler.HandleRequest(req);

  EXPECT_EQ(resp.status, StatusCode::OK);

  std::ostringstream oss;

  // reconstruct headers mimicking internal behavior
  oss << req.method << " " << req.path << " " << req.version << "\r\n";
  for (auto &hdr : req.headers) {
    oss << hdr.first << ": " << hdr.second << "\r\n";
  }
  oss << "\r\n" << req.body;
  const auto expected = oss.str();

  EXPECT_EQ(resp.body, expected);
  EXPECT_EQ(resp.headers.at("Content-Type"), "text/plain");
  EXPECT_EQ(resp.headers.at("Content-Length"), std::to_string(expected.size()));
}

// tests that the handler correctly rejects any non-GET request
TEST(EchoHandlerTest, RejectsNonGet) {
  EchoHandler eHandler("/echo");
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
    HttpResponse resp = eHandler.HandleRequest(req);

    EXPECT_EQ(resp.status, StatusCode::BAD_REQUEST);
    EXPECT_EQ(resp.body, "");
    EXPECT_EQ(resp.headers.at("Content-Type"), "text/plain");
    EXPECT_EQ(resp.headers.at("Content-Length"), "0");
  }
}
