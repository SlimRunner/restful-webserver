#include "session.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

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
    using session::request_buffer_;
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
    MOCK_METHOD(std::shared_ptr<HttpResponse>, handle_request, (const HttpRequest &), (override));
    MOCK_METHOD(bool, can_handle, (const std::string &), (const, override));
    MOCK_METHOD(std::string, get_prefix, (), (const, override));
};

class SessionTest : public ::testing::Test {
   protected:
    boost::asio::io_service io_service_;
    std::shared_ptr<MockHandler> mock_handler_;
    std::vector<std::shared_ptr<RequestHandler>> handlers_;

    std::unique_ptr<TestableSession, session_deleter> session_;

    void SetUp() override {
        mock_handler_ = std::make_shared<MockHandler>();
        handlers_.push_back(mock_handler_);
        session_ = std::unique_ptr<TestableSession, session_deleter>(
            new TestableSession(io_service_, handlers_), session_deleter());

        ON_CALL(*mock_handler_, get_prefix()).WillByDefault(testing::Return("/echo"));

        ON_CALL(*mock_handler_, handle_request(testing::_))
            .WillByDefault(testing::Invoke([](const HttpRequest &req) {
                auto res = std::make_shared<HttpResponse>();
                res->body = "mock";
                if (req.method == "GET") {
                    res->status = StatusCode::OK;
                }else{
                    res->status = StatusCode::BAD_REQUEST;
                }
                res->headers["Content-Type"] = "text/plain";
                res->headers["Content-Length"] = std::to_string(res->body.size());
                return res;  
            }));
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
TEST_F(SessionTest, HandleReadCompleteRequest) {
    std::string fullReq =
        "GET /echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n\r\n";
    boost::system::error_code successCode{};
    const std::string mockRes =
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Length: 4\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "mock";

    //Tell the mock to claim this path
    EXPECT_CALL(*mock_handler_, can_handle("/echo"))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*mock_handler_, handle_request(testing::_))
        .Times(1);

    session_->set_request(fullReq);
    session_->handle_read(successCode, fullReq.size());

    EXPECT_FALSE(session_->already_deleted_);
    EXPECT_EQ(session_->current_response_, mockRes);
}

// uses a mock response to test not handled (404)
TEST_F(SessionTest, HandleReadUnhandledRequest) {
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
TEST_F(SessionTest, HandleMultipleRequests) {
    std::string fullReq =
        "GET /echo HTTP/1.1\r\n"
        "Host: localhost\r\n\r\n"
        "GET /echo HTTP/1.1\r\n"
        "Host: localhost\r\n\r\n";
    boost::system::error_code successCode{};
    const std::string mockRes =
       "HTTP/1.1 200 OK\r\n"
        "Connection: keep-alive\r\n"
        "Content-Length: 4\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "mock";
        
    EXPECT_CALL(*mock_handler_, can_handle("/echo"))
        .Times(2)
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mock_handler_, handle_request(testing::_))
        .Times(2);  // Two requests
    session_->set_request(fullReq);
    session_->handle_read(successCode, fullReq.size());
    EXPECT_FALSE(session_->already_deleted_);
    EXPECT_EQ(session_->current_response_, mockRes);
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
    auto mock_handler_long = std::make_shared<MockHandler>();
    handlers_.push_back(mock_handler_long);

    // Recreate session to use updated handlers
    session_ = std::unique_ptr<TestableSession, session_deleter>(
        new TestableSession(io_service_, handlers_), session_deleter());

    // Short prefix handler
    EXPECT_CALL(*mock_handler_, can_handle("/echo/long/path"))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*mock_handler_, get_prefix())
        .WillOnce(testing::Return("/echo"));

    // Longer prefix handler
    EXPECT_CALL(*mock_handler_long, can_handle("/echo/long/path"))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*mock_handler_long, get_prefix())
        .WillOnce(testing::Return("/echo/long"));
    EXPECT_CALL(*mock_handler_long, handle_request(testing::_))
        .Times(1)
        .WillOnce(testing::Invoke([](const HttpRequest& req) {
            auto res = std::make_shared<HttpResponse>();
            res->status = StatusCode::OK;
            res->body = "matched long";
            res->headers["Content-Type"] = "text/plain";
            res->headers["Content-Length"] = "12";
            return res;
        }));

    std::string request_str =
        "GET /echo/long/path HTTP/1.1\r\n"
        "Host: localhost\r\n\r\n";

    session_->set_request(request_str);
    boost::system::error_code success{};
    session_->handle_read(success, request_str.size());

    EXPECT_FALSE(session_->already_deleted_);
    EXPECT_NE(session_->current_response_.find("matched long"), std::string::npos);
}

// Ensure /echoes does not match /echo.
TEST_F(SessionTest, PrefixShadowing_EchoDoesNotMatchEchoes) {
    auto shadowed_handler = std::make_shared<MockHandler>();
    handlers_.push_back(shadowed_handler);

    // Recreate session with both handlers
    session_ = std::unique_ptr<TestableSession, session_deleter>(
        new TestableSession(io_service_, handlers_), session_deleter());

    // Only the shadowed handler claims `/echoes`, but not `/echo`
    EXPECT_CALL(*mock_handler_, can_handle("/echoes"))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*shadowed_handler, can_handle("/echoes"))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*shadowed_handler, get_prefix())
        .WillOnce(testing::Return("/echoes"));
    EXPECT_CALL(*shadowed_handler, handle_request(testing::_))
        .WillOnce(testing::Invoke([](const HttpRequest& req) {
            auto res = std::make_shared<HttpResponse>();
            res->status = StatusCode::OK;
            res->body = "shadowed";
            res->headers["Content-Type"] = "text/plain";
            res->headers["Content-Length"] = "9";
            return res;
        }));

    std::string request =
        "GET /echoes HTTP/1.1\r\n"
        "Host: localhost\r\n\r\n";

    session_->set_request(request);
    boost::system::error_code success{};
    session_->handle_read(success, request.size());

    EXPECT_FALSE(session_->already_deleted_);
    EXPECT_NE(session_->current_response_.find("shadowed"), std::string::npos);
}

//No Match at All -> Should return 404
TEST_F(SessionTest, NoMatchingPrefixReturns404) {
    EXPECT_CALL(*mock_handler_, can_handle("/random"))
        .WillOnce(testing::Return(false));  // No match

    std::string request =
        "GET /random HTTP/1.1\r\n"
        "Host: localhost\r\n\r\n";

    session_->set_request(request);
    boost::system::error_code success{};
    session_->handle_read(success, request.size());

    EXPECT_FALSE(session_->already_deleted_);
    EXPECT_NE(session_->current_response_.find("404 Not Found"), std::string::npos);
}

//Tie on Prefix Length choosing the first one wins (or deterministic winner)
TEST_F(SessionTest, SameLengthPrefixPicksFirstRegistered) {
    auto second_handler = std::make_shared<MockHandler>();
    handlers_.push_back(second_handler);

    session_ = std::unique_ptr<TestableSession, session_deleter>(
        new TestableSession(io_service_, handlers_), session_deleter());

    // Both claim same-length prefix
    EXPECT_CALL(*mock_handler_, can_handle("/tie"))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*mock_handler_, get_prefix())
        .WillOnce(testing::Return("/tie"));

    EXPECT_CALL(*second_handler, can_handle("/tie"))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*second_handler, get_prefix())
        .WillOnce(testing::Return("/tie"));

    EXPECT_CALL(*mock_handler_, handle_request(testing::_))
        .WillOnce(testing::Invoke([](const HttpRequest& req) {
            auto res = std::make_shared<HttpResponse>();
            res->status = StatusCode::OK;
            res->body = "first wins";
            res->headers["Content-Type"] = "text/plain";
            res->headers["Content-Length"] = "10";
            return res;
        }));

    std::string request =
        "GET /tie HTTP/1.1\r\n"
        "Host: localhost\r\n\r\n";

    session_->set_request(request);
    boost::system::error_code success{};
    session_->handle_read(success, request.size());

    EXPECT_FALSE(session_->already_deleted_);
    EXPECT_NE(session_->current_response_.find("first wins"), std::string::npos);
}
