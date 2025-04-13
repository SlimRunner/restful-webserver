#include "gtest/gtest.h"
#include "session.h"

class SessionTest : public ::testing::Test {
protected:
  void SetUp() override {
    // No-op: we won't use a real socket for this unit test
  }

  session* create_test_session() {
    // Dummy io_service just to construct the session
    static boost::asio::io_service fake_service;
    return new session(fake_service);
  }
};

TEST_F(SessionTest, HandlesCompleteHttpRequest) {
  session* s = create_test_session();

  // Simulate receiving a complete HTTP GET request
  std::string fake_request =
    "GET / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "User-Agent: test-client\r\n"
    "\r\n";

  // Pretend this was the full request
  s->set_request(fake_request);

  // Generate the expected HTTP response manually
  std::string expected_response_body = fake_request;
  std::string expected_response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: " + std::to_string(expected_response_body.size()) + "\r\n"
    "\r\n" +
    expected_response_body;

  // Instead of invoking boost::asio::async_write,
  // let’s assume we have a method that builds the response string
  std::string actual_response = s->build_response();  // <- You’ll need to expose this

  EXPECT_EQ(actual_response, expected_response);

  delete s;
}
