#include "server.h"

#include <boost/bind/bind.hpp>
#include <boost/log/trivial.hpp>
using namespace boost::placeholders;

/*
Constructs the server, binds to the given port, and starts accepting connections.
- io_service: Boost I/O context for async operations.
- port: Port number to listen on.
- handlers: List of request handlers.
- factory: Function used to create session objects (can be mocked for testing).
*/
server::server(boost::asio::io_service &io_service, short port, RoutingMap routes,
               SessionFactory factory)
    : io_service_(io_service),
      acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
      // Save the provided factory function so it can be used later in start_accept()
      session_factory_(factory),
      routes_(routes) {
    BOOST_LOG_TRIVIAL(info) << "Server constructor: Listening on port " << port;

    // Start accepting incoming client connections
    start_accept();
}

/*
Starts an asynchronous accept operation to wait for incoming client connections.
On connection, a new session is created and passed to handle_accept().
*/
void server::start_accept() {
    BOOST_LOG_TRIVIAL(debug) << "Waiting for new client connection...";

    // Create a new session using the injected factory (allows unit test injection of mocks)
    session *new_session = session_factory_(io_service_, routes_);

    // Initiate asynchronous accept operation; when a client connects, handle_accept will be called
    acceptor_.async_accept(
        new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session, boost::asio::placeholders::error));
}

/*
Handles a new incoming client connection.
- new_session: The session object for the connected client.
- error: Error code from Boost.Asio's accept operation.
Starts the session if no error, otherwise deletes it. Always re-initiates accept().
*/
void server::handle_accept(session *new_session, const boost::system::error_code &error) {
    if (!error) {
        try {
            // Log the IP address of the connected client
            std::string client_ip = new_session->socket().remote_endpoint().address().to_string();
            BOOST_LOG_TRIVIAL(info) << "Accepted connection from " << client_ip;
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(warning)
                << "Accepted connection, but failed to retrieve client IP: " << e.what();
        }
        // Begin processing requests from this session
        new_session->start();
    } else {
        BOOST_LOG_TRIVIAL(error) << "Failed to accept connection: " << error.message();
        // Clean up the failed session to avoid memory leaks
        delete new_session;
    }
    // Continue accepting new connections
    start_accept();
}
