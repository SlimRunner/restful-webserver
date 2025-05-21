#include "session.h"

#include "echo_handler.h"
#include "entity_handler.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "static_file_handler.h"

extern volatile int force_link_echo_handler;
extern volatile int force_link_static_handler;
extern volatile int force_link_entity_handler;

// Made a testableSession class to test the private stuff in session class
class TestableSession : public session {
   public:
    using session::session;  // inherit constructor

    using session::handle_read;   // expose handle_read
    using session::handle_write;  // expose handle_write
    using session::ParseRequest;  // expose ParseRequest
    using session::safe_delete;
    using session::socket;

    // Provide access to the internal data
    using session::already_deleted_;
    using session::current_response_;
    using session::data_;
    using session::registry_;
    using session::request_buffer_;
    using session::routes_;
};

struct session_deleter {
    void operator()(TestableSession *sessionPtr) {
        if (sessionPtr) {
            if (!sessionPtr->already_deleted_) {
                delete sessionPtr;
            }
            // else: it already called delete this; inside safe_delete()
        }
    }
};

class MockHandler : public RequestHandler {
   public:
    std::string path_prefix_;
    MockHandler(const std::string &path_prefix, const std::map<std::string, std::string> &_)
        : path_prefix_{path_prefix} {}

    std::unique_ptr<HttpResponse> handle_request(const HttpRequest &request) override {
        HttpResponse res;
        res.status = StatusCode::OK;  // Set status to 200 OK
        res.body = path_prefix_;      // Keep path prefix for verification
        res.headers["Content-Type"] = "text/plain";
        res.headers["Content-Length"] = std::to_string(res.body.size());
        return std::make_unique<HttpResponse>(res);
    }
};

class MockRegistry : public IHandlerRegistry {
   public:
    MOCK_METHOD(void, register_handler, (const std::string &name, RequestHandlerFactory factory),
                (override));
    MOCK_METHOD(RequestHandlerFactory, get_factory, (const std::string &name), (const override));

    MockRegistry() {
        // Default behavior: return MockHandler
        ON_CALL(*this, get_factory).WillByDefault([this](const std::string &name) {
            return [name](const std::string &path_prefix,
                          const std::map<std::string, std::string> &args) {
                return std::make_shared<MockHandler>(path_prefix, args);
            };
        });
    }
};

class SessionTest : public ::testing::Test {
   protected:
    boost::asio::io_service io_service_;

    MockRegistry mock_registry;

    std::unique_ptr<TestableSession, session_deleter> session_;

    void SetUp() override {
        (void)force_link_echo_handler;
        (void)force_link_static_handler;
        (void)force_link_entity_handler;

        RoutingMap routes_;

        session_ = std::unique_ptr<TestableSession, session_deleter>(
            new TestableSession(io_service_, routes_, &mock_registry), session_deleter());
    }

    void injectData(const std::string &req) {
        session_->request_buffer_ = req;
    }

    void runIO() {
        // Useful if you're testing async behavior
        io_service_.run();
    }
};

class SessionTestReal : public ::testing::Test {
   protected:
    boost::asio::io_service io_service_;

    std::unique_ptr<TestableSession, session_deleter> session_;

    void SetUp() override {
        (void)force_link_echo_handler;
        (void)force_link_static_handler;
        (void)force_link_entity_handler;

        IHandlerRegistry *registry = &HandlerRegistry::instance();
        RoutingMap routes_;

        // no need to use other handlers because they are tested in a
        // different unit
        routes_.insert({"/echo", {{"EchoHandler"}, {}}});

        session_ = std::unique_ptr<TestableSession, session_deleter>(
            new TestableSession(io_service_, routes_, registry), session_deleter());
    }

    void injectData(const std::string &req) {
        session_->request_buffer_ = req;
    }

    void runIO() {
        // Useful if you're testing async behavior
        io_service_.run();
    }
};

TEST_F(SessionTest, ParseValidHttpRequest) {
    std::string request = "GET /hello HTTP/1.1\r\nHost: example.com\r\n\r\n";
    HttpRequest parsed = session_->ParseRequest(request);

    EXPECT_EQ(parsed.method, "GET");
    EXPECT_EQ(parsed.path, "/hello");
    EXPECT_EQ(parsed.version, "HTTP/1.1");
    EXPECT_EQ(parsed.headers["Host"], "example.com");
}

// it test multiple related things
// - that a line with only \r stops parsing
// - that trailing spaces after : are trimmed
// - line with no \r takes different branch
// - leading tabs are also trimmed
TEST_F(SessionTest, ParseHttpEdgeCases) {
    std::string request =
        "GET /hello HTTP/1.1\r\n"
        "Host:      example.com\n"    // no trailing r
        "User-Agent:\ttest-client\n"  // trim leading tab
        "no_colon\r\n"                // invalid header is ignored
        "key: value\r\n"              // to check parsing continued
        "\r\n"                        // stops parsing
        "asd:qwe\r\n\r\n";            // never reached
    HttpRequest parsed = session_->ParseRequest(request);

    std::map<std::string, std::string> expectedHeader = {
        {"Host", "example.com"}, {"User-Agent", "test-client"}, {"key", "value"},
        // the \r on its own causes the loop to terminate
    };

    EXPECT_EQ(parsed.method, "GET");
    EXPECT_EQ(parsed.path, "/hello");
    EXPECT_EQ(parsed.version, "HTTP/1.1");
    EXPECT_EQ(parsed.headers, expectedHeader);
}

// test safe delete
TEST_F(SessionTest, SafeSessionDeletion) {
    // --
    session_->safe_delete();
    EXPECT_TRUE(session_->already_deleted_);
    // make sure we do not get a seg fault
    session_->safe_delete();
}

// tests that port can be opened and closed correctly
TEST_F(SessionTest, SocketOpensAndCloses) {
    session_->socket().open(boost::asio::ip::tcp::v4());
    EXPECT_TRUE(session_->socket().is_open());
    session_->socket().close();
    EXPECT_FALSE(session_->already_deleted_);
}

// tests that session starts without crashing
TEST_F(SessionTest, SessionStartsCleanly) {
    session_->start();
}

// tests that set_requests works correctly
TEST_F(SessionTest, SessionDirectSetRequest) {
    // Simulate receiving a complete HTTP GET request
    std::string fake_request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "User-Agent: test-client\r\n"
        "\r\n";

    session_->set_request(fake_request);
    EXPECT_EQ(session_->request_buffer_, fake_request);
    EXPECT_FALSE(session_->already_deleted_);
}

// tests for handle_write behavior
TEST_F(SessionTest, HandleWriteWithError) {
    // Simulate a write error; should trigger safe_delete()
    boost::system::error_code errCode(1, boost::system::system_category());
    session_->handle_write(errCode);
    EXPECT_TRUE(session_->already_deleted_);
}

TEST_F(SessionTest, HandleWriteCloseConnection) {
    // Closed socket with no error should trigger safe_delete()
    session_->socket().open(boost::asio::ip::tcp::v4());
    session_->socket().close();
    boost::system::error_code successCode{};
    session_->handle_write(successCode);
    EXPECT_TRUE(session_->already_deleted_);
}

TEST_F(SessionTest, HandleWriteContinueReading) {
    // Open socket with no error should NOT delete session
    session_->socket().open(boost::asio::ip::tcp::v4());
    boost::system::error_code successCode{};
    session_->handle_write(successCode);
    EXPECT_FALSE(session_->already_deleted_);
}

// tests for handle_read behavior on error
TEST_F(SessionTest, HandleReadWithError) {
    // Simulate a read error; should trigger safe_delete()
    boost::system::error_code errCode(1, boost::system::generic_category());
    session_->handle_read(errCode, 0);
    EXPECT_TRUE(session_->already_deleted_);
}

// sends an oversize payload to get handle_read to fail
TEST_F(SessionTest, HandleReadWithOversize) {
    std::string oversized(9000, ' ');
    boost::system::error_code successCode{};
    session_->set_request(oversized);
    session_->handle_read(successCode, oversized.size());
    EXPECT_FALSE(session_->already_deleted_);
    constexpr auto expectedResp =
        "HTTP/1.1 413 Payload Too Large\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n";
    EXPECT_EQ(session_->current_response_, expectedResp);
}

// uses a mock response to test a correct handle read
TEST_F(SessionTestReal, HandleReadCompleteRequest) {
    std::string fullReq =
        "GET /echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n\r\n";
    boost::system::error_code successCode{};

    session_->set_request(fullReq);
    session_->handle_read(successCode, fullReq.size());

    EXPECT_FALSE(session_->already_deleted_);
    EXPECT_NE(session_->current_response_.find("200 OK"), std::string::npos);
    EXPECT_NE(session_->current_response_.find("Connection: close"), std::string::npos);
}

// uses a mock response to test not handled (404)
TEST_F(SessionTestReal, HandleReadUnhandledRequest) {
    std::string fullReq =
        "GET /mockfails HTTP/1.1\r\n"
        "Host: localhost\r\n\r\n";
    boost::system::error_code successCode{};

    session_->set_request(fullReq);
    session_->handle_read(successCode, fullReq.size());
    EXPECT_FALSE(session_->already_deleted_);
    EXPECT_NE(session_->current_response_.find("404 Not Found"), std::string::npos);
}

// uses a mock response to test multiple requests
TEST_F(SessionTestReal, HandleMultipleRequests) {
    std::string fullReq =
        "GET /echo HTTP/1.1\r\n"
        "Host: localhost\r\n\r\n"
        "GET /echo HTTP/1.1\r\n"
        "Host: localhost\r\n\r\n";
    boost::system::error_code successCode{};

    session_->set_request(fullReq);
    session_->handle_read(successCode, fullReq.size());
    EXPECT_FALSE(session_->already_deleted_);
    EXPECT_NE(session_->current_response_.find("200 OK"), std::string::npos);
    EXPECT_NE(session_->current_response_.find("Connection: keep-alive"), std::string::npos);
}

// tests that it fails when not HTTP1.1
TEST_F(SessionTest, HandleBadVersionRequest) {
    std::string fullReq =
        "GET /echo HTTP/2.0\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n";
    boost::system::error_code successCode{};
    const std::string actualRes =
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Length: 0\r\n"
        "Content-Type: text/plain\r\n\r\n";

    session_->set_request(fullReq);
    session_->handle_read(successCode, fullReq.size());
    EXPECT_FALSE(session_->already_deleted_);
    EXPECT_EQ(session_->current_response_, actualRes);
}

// This tests the longest matching prefix
// Using a mockhandler to achieve this by maching a handler take the paths
TEST_F(SessionTest, PicksLongestMatchingPrefix) {
    session_->routes_.insert({"/", {{"/"}, {}}});
    session_->routes_.insert({"/A", {{"/A"}, {}}});
    session_->routes_.insert({"/A/D/C", {{"/A/D/C"}, {}}});
    session_->routes_.insert({"/A/B/C", {{"/A/B/C"}, {}}});
    session_->routes_.insert({"/A/B/C/D/E/F/G", {{"/A/B/C/D/E/F/G"}, {}}});

    std::string fullReq =
        "GET /A/B/C/D/file.txt HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    boost::system::error_code successCode{};

    session_->set_request(fullReq);
    session_->handle_read(successCode, fullReq.size());
    EXPECT_FALSE(session_->already_deleted_);
    std::string expected = "/A/B/C";
    std::string response = session_->current_response_;
    EXPECT_GE(response.size(), expected.size());
    EXPECT_EQ(response.substr(response.size() - expected.size()), expected);

    fullReq =
        "GET /A/B/C/D/E/F/G/more/paths/file.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    session_->set_request(fullReq);
    session_->handle_read(successCode, fullReq.size());
    EXPECT_FALSE(session_->already_deleted_);
    expected = "/A/B/C/D/E/F/G";
    response = session_->current_response_;
    EXPECT_GE(response.size(), expected.size());
    EXPECT_EQ(response.substr(response.size() - expected.size()), expected);

    fullReq =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    session_->set_request(fullReq);
    session_->handle_read(successCode, fullReq.size());
    EXPECT_FALSE(session_->already_deleted_);
    expected = "/";
    response = session_->current_response_;
    EXPECT_GE(response.size(), expected.size());
    EXPECT_EQ(response.substr(response.size() - expected.size()), expected);
}

// Ensure /echoes does not match /echo.
TEST_F(SessionTest, PrefixShadowing_EchoDoesNotMatchEchoes) {
    session_->routes_.insert({"/echo", {{"/echo"}, {}}});
    session_->routes_.insert({"/echoes", {{"/echoes"}, {}}});

    std::string fullReq =
        "GET /echoes HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    boost::system::error_code successCode{};

    session_->set_request(fullReq);
    session_->handle_read(successCode, fullReq.size());
    EXPECT_FALSE(session_->already_deleted_);
    std::string expected = "/echoes";
    std::string response = session_->current_response_;
    EXPECT_GE(response.size(), expected.size());
    EXPECT_EQ(response.substr(response.size() - expected.size()), expected);
}

// No Match at All -> Should return 404
TEST_F(SessionTest, NoMatchingPrefixReturns404) {
    session_->routes_.insert({"/A/D/C", {{"/A/D/C"}, {}}});
    session_->routes_.insert({"/A/B/C", {{"/A/B/C"}, {}}});
    session_->routes_.insert({"/A/B/C/D/E/F/G", {{"/A/B/C/D/E/F/G"}, {}}});

    std::string fullReq =
        "GET /A/D/B/E/file.txt HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    boost::system::error_code successCode{};

    session_->set_request(fullReq);
    session_->handle_read(successCode, fullReq.size());
    EXPECT_FALSE(session_->already_deleted_);
    EXPECT_NE(session_->current_response_.find("404 Not Found"), std::string::npos);
}
