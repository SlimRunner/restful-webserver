#include "session.h"
#include <boost/bind/bind.hpp>
#include <sstream>
#include <string>
using namespace boost::placeholders;

session::session(boost::asio::io_service& io_service)
  //adding already_deleted_ member to constructor 
  : socket_(io_service), already_deleted_(false) {} 

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
// Since the server can handle partial requests and build_response()
// now depends on each individual full_request, update it to accept a parameter
std::string session::build_response(const std::string& full_request) {
  return "HTTP/1.1 200 OK\r\n"
         "Content-Type: text/plain\r\n"
         "Content-Length: " + std::to_string(full_request.size()) + "\r\n"
         "\r\n" + full_request;
}
// Helper function to set the request member. Used for testing.
void session::set_request(const std::string& req) {
  request_buffer_ = req;
}

// Helper function to prevent freeing the same memory twice
// as deletion is done in handle_read and handle_write methods
void session::safe_delete() {
  if (!already_deleted_) {
    already_deleted_ = true;
    delete this;
  }
}

void session::handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
  if (error) {
    safe_delete(); // to avoid Double Free Error
    return;
  }
  // Before appending, check if it will exceed a defined maximum buffer size
  // buffer size limit defined in session.h, currently 8192 bytes = 8 KB
  if (request_buffer_.size() + bytes_transferred > max_request_size_) {
    std::string error_response =
      "HTTP/1.1 413 Payload Too Large\r\n"
      "Content-Length: 0\r\n"
      "Connection: close\r\n"
      "\r\n";

    boost::asio::async_write(socket_,
      boost::asio::buffer(error_response),
      boost::bind(&session::handle_write, this, _1));

    return;
  }
  // Accumulate data in request_buffer_
  request_buffer_ += std::string(data_, bytes_transferred);

  while (true) {
    size_t header_end = request_buffer_.find("\r\n\r\n");
    if (header_end == std::string::npos) {
      break;  // wait for complete request
    }

    // Extract one full request
    std::string full_request = request_buffer_.substr(0, header_end + 4);
    request_buffer_ = request_buffer_.substr(header_end + 4);  // keep remainder

    // Parse method line
    std::istringstream stream(full_request);
    std::string method, path, version;
    stream >> method >> path >> version;

    std::string response;
    bool close_connection = full_request.find("Connection: close") != std::string::npos;

    if (method != "GET") {
      response =
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Length: 0\r\n"
        "Content-Type: text/plain\r\n"
        + std::string(close_connection ? "Connection: close\r\n" : "") +
        "\r\n";
    } else {
      std::string response_body = full_request;
      response = build_response(full_request);
      if (close_connection) {
        response.insert(response.find("\r\n\r\n"), "\r\nConnection: close");
      }
    }

    boost::asio::async_write(socket_,
      boost::asio::buffer(response),
      boost::bind(&session::handle_write, this, _1));

    if (close_connection) {
      return; // Don't read anymore
    }
  }

  // Continue reading for more requests
  socket_.async_read_some(boost::asio::buffer(data_, max_length),
    boost::bind(&session::handle_read, this, _1, _2));
}

void session::handle_write(const boost::system::error_code& error) {
  if (!error) {
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
      boost::bind(&session::handle_read, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
  }
  else {
    safe_delete(); // to avoid Double Free Error
  }
}
