#include "session.h"
#include <boost/bind/bind.hpp>
using namespace boost::placeholders;

session::session(boost::asio::io_service& io_service)
  : socket_(io_service) {}

tcp::socket& session::socket() {
  return socket_;
}

void session::start() {
  socket_.async_read_some(boost::asio::buffer(data_, max_length),
    boost::bind(&session::handle_read, this,
      boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred));
}

// Helper function to construct the request's response
std::string session::build_response() {
  return "HTTP/1.1 200 OK\r\n"
         "Content-Type: text/plain\r\n"
         "Content-Length: " + std::to_string(request_.size()) + "\r\n"
         "\r\n" +
         request_;
}
// Helper function to set the request member. Used for testing.
void session::set_request(const std::string& req) {
  request_ = req;
}
void session::handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
  if (!error) {
    // Append received data to buffer
    request_ += std::string(data_, bytes_transferred);

    // Check for end of HTTP headers
    if (request_.find("\r\n\r\n") != std::string::npos) {
      std::string response_body = request_;
      std::string response = build_response();

      boost::asio::async_write(socket_,
        boost::asio::buffer(response),
        boost::bind(&session::handle_write, this,
          boost::asio::placeholders::error));
    } else {
      // Keep reading until request is complete
      socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
    }
  } else {
    delete this;
  }
}

void session::handle_write(const boost::system::error_code& error) {
  if (!error) {
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
      boost::bind(&session::handle_read, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
  }
  else {
    delete this;
  }
}
