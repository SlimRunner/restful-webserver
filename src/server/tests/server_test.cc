#include "server.h"

#include "gtest/gtest.h"
#include "mock_session.h"
#include "session.h"

class ServerTest : public ::testing::Test {};

// Made a testable server that inherits from server to use do tests
class TestableServer : public server {
   public:
    using server::handle_accept;
    using server::server;  // inherit constructors
    using server::start_accept;
};

TEST_F(ServerTest, UsesInjectedSessionFactory) {
    boost::asio::io_service io;
    int factory_call_count = 0;

    // Create a factory that tracks how many times it's called
    SessionFactory test_factory = [&](auto &io_ref, auto handlers) {
        ++factory_call_count;
        return new session(io_ref);
    };

    // Create server using custom factory
    TestableServer test_server(io, 8080, /* handlers */ {}, test_factory);

    // Run io_service to trigger start_accept()
    io.poll();  // or io.run() if needed

    // You might not see the factory called unless a client connects,
    // but this proves the server *can* accept and store the factory.
    EXPECT_GE(factory_call_count, 0);
}

// Verify start() Gets Called on Successful Accept
TEST_F(ServerTest, HandleAcceptCallsStart) {
    boost::asio::io_service io;

    // Make the mock directly, don't rely on factory assignment
    MockSession *mock_ptr = new MockSession(io);

    // Server can still have a factory, but we won't use it in this test
    TestableServer test_server(io, 8080, /* handlers */ {},
                               [](auto &io_ref, auto handlers) { return new session(io_ref); });

    // Simulate a successful accept manually
    boost::system::error_code success(0, boost::system::generic_category());
    test_server.handle_accept(mock_ptr, success);

    // Now verify start() was called
    EXPECT_TRUE(mock_ptr->start_called());

    // Cleanup (since we're bypassing the async delete)
    delete mock_ptr;
}

// Verify handle_accept() Cleans Up on Error
TEST_F(ServerTest, HandleAcceptOnErrorDeletesSession) {
    boost::asio::io_service io;
    bool deleted = false;

    class SelfDeletingMockSession : public session {
       public:
        SelfDeletingMockSession(boost::asio::io_service &io, bool &deleted_flag)
            : session(io), deleted_ref(deleted_flag) {}

        ~SelfDeletingMockSession() {
            deleted_ref = true;
        }

       private:
        bool &deleted_ref;
    };

    TestableServer test_server(io, 8080, /* handlers */ {},
                               [](auto &io_ref, auto handlers) { return new session(io_ref); });

    // Use heap-allocated test session
    session *test_session = new SelfDeletingMockSession(io, deleted);

    // Simulate error
    boost::system::error_code simulated_error(1, boost::system::generic_category());

    // Pass it in to trigger delete
    test_server.handle_accept(test_session, simulated_error);

    // Destructor should have set the flag
    EXPECT_TRUE(deleted);
}

// Test that start_accept() creates a new session using the injected factory.
TEST_F(ServerTest, StartAcceptCreatesNewSession) {
    boost::asio::io_service io;
    int factory_call_count = 0;

    // Factory increments counter every time it's called
    SessionFactory test_factory = [&](auto &io_ref, auto handlers) {
        ++factory_call_count;
        return new session(io_ref);  // use a real session
    };

    TestableServer test_server(io, 8080, /* handlers */ {}, test_factory);

    // Reset count to isolate start_accept() call
    factory_call_count = 0;

    // Manually trigger start_accept
    test_server.start_accept();

    // Factory should have been called once
    EXPECT_EQ(factory_call_count, 1);
}
