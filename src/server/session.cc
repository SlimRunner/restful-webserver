#include "session.h"
#include <boost/bind/bind.hpp>
#include <sstream>
#include <string>
#include <iostream>
#include <boost/log/trivial.hpp>
using namespace boost::placeholders;

session::session(boost::asio::io_service &io_service, std::vector<std::shared_ptr<RequestHandler>> handlers)
    // adding already_deleted_ member to constructor
    : socket_(io_service), handlers_(handlers), already_deleted_(false)
{
}

tcp::socket &session::socket()
{
  return socket_;
}

void session::start()
{
  BOOST_LOG_TRIVIAL(debug) << "Session started: waiting for request";
  socket_.async_read_some(boost::asio::buffer(data_, max_length),
                          boost::bind(&session::handle_read, this,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred));
}

// Helper function to set the request member. Used for testing.
void session::set_request(const std::string &req)
{
  request_buffer_ = req;
}

// Helper function to prevent freeing the same memory twice
// as deletion is done in handle_read and handle_write methods
void session::safe_delete()
{
  if (!already_deleted_)
  {
    BOOST_LOG_TRIVIAL(debug) << "Session deleted.";
    already_deleted_ = true;
    delete this;
  }
}

HttpRequest session::ParseRequest(const std::string &request_str)
{
  HttpRequest request;
  std::istringstream stream(request_str);

  // Parse request line
  stream >> request.method >> request.path >> request.version;

  // Parse headers
  std::string line;
  std::getline(stream, line); // Consume the rest of the first line

  while (std::getline(stream, line) && line != "\r")
  {
    size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos)
    {
      std::string header_name = line.substr(0, colon_pos);
      std::string header_value = line.substr(colon_pos + 1);

      // Trim whitespace
      while (!header_value.empty() && (header_value[0] == ' ' || header_value[0] == '\t'))
        header_value.erase(0, 1);

      // Remove trailing \r if present
      if (!header_value.empty() && header_value[header_value.length() - 1] == '\r')
        header_value.erase(header_value.length() - 1);

      request.headers[header_name] = header_value;
    }
  }

  // Parse body (if any)
  std::stringstream body_stream;
  body_stream << stream.rdbuf();
  request.body = body_stream.str();

  return request;
}

void session::handle_read(const boost::system::error_code &error, size_t bytes_transferred)
{
  if (error)
  {
    BOOST_LOG_TRIVIAL(error) << "Socket read error: " << error.message();
    safe_delete(); // to avoid Double Free Error
    return;
  }
  // Before appending, check if it will exceed a defined maximum buffer size
  // buffer size limit defined in session.h, currently 8192 bytes = 8 KB
  if (request_buffer_.size() + bytes_transferred > max_request_size_)
  {
    BOOST_LOG_TRIVIAL(warning) << "Payload too large: " << request_buffer_.size() + bytes_transferred << " bytes";
    std::string error_response =
        "HTTP/1.1 413 Payload Too Large\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n";

    current_response_ = error_response; // Store the response for logging
    boost::asio::async_write(socket_,
                             boost::asio::buffer(error_response),
                             boost::bind(&session::handle_write, this, _1));

    return;
  }
  // Accumulate data in request_buffer_
  request_buffer_ += std::string(data_, bytes_transferred);

  while (true)
  {
    size_t header_end = request_buffer_.find("\r\n\r\n");
    if (header_end == std::string::npos)
    {
      break; // wait for complete request
    }

    // Extract one full request
    std::string full_request = request_buffer_.substr(0, header_end + 4);
    request_buffer_ = request_buffer_.substr(header_end + 4); // keep remainder

    // Parse the HTTP request

    HttpRequest request = ParseRequest(full_request);

    // Check HTTP version - only accepts HTTP/1.1
    if (request.version != "HTTP/1.1")
    {
      BOOST_LOG_TRIVIAL(warning) << "Unsupported HTTP version: " << request.version;
      HttpResponse response;
      response.status = StatusCode::BAD_REQUEST;
      response.body = "";
      response.headers["Content-Type"] = "text/plain";
      response.headers["Content-Length"] = std::to_string(response.body.size());
      current_response_ = response.ToString();
      boost::asio::async_write(socket_,
                               boost::asio::buffer(current_response_),
                               boost::bind(&session::handle_write, this, _1));
      return;
    }

    try
    {
      std::string client_ip = socket_.remote_endpoint().address().to_string();
      BOOST_LOG_TRIVIAL(info) << "Received " << request.method << " " << request.path << " from " << client_ip;
    }
    catch (...)
    {
      BOOST_LOG_TRIVIAL(info) << "Received " << request.method << " " << request.path << " from [unknown IP]";
    }
    // Find the appropriate handler for the request path
    bool handled = false;
    HttpResponse response;

    for (auto &handler : handlers_)
    {
      if (handler->CanHandle(request.path))
      {
        BOOST_LOG_TRIVIAL(debug) << "Handler matched for path: " << request.path;
        response = handler->HandleRequest(request);
        handled = true;
        break;
      }
    }

    // If no handler was found, return a 404 Not Found response
    if (!handled)
    {
      BOOST_LOG_TRIVIAL(warning) << "No handler for path: " << request.path << ". Sending 404.";

      response.status = StatusCode::NOT_FOUND;
      response.body = "404 Not Found";
      response.headers["Content-Type"] = "text/plain";
      response.headers["Content-Length"] = std::to_string(response.body.size());
    }

    // Check if the connection should be closed
    bool close_connection = (request.headers.find("Connection") != request.headers.end() &&
                             request.headers["Connection"] == "close");

    if (close_connection)
    {
      response.headers["Connection"] = "close";
    }
    else
    {
      response.headers["Connection"] = "keep-alive";
    }

    // Convert response to string and store it in the member variable
    current_response_ = response.ToString();
    BOOST_LOG_TRIVIAL(debug) << "Sent response for " << request.path << " with status: " << response.status;
    boost::asio::async_write(socket_,
                             boost::asio::buffer(current_response_),
                             boost::bind(&session::handle_write, this, _1));

    if (close_connection)
    {
      return; // Don't read anymore
    }
  }

  // Continue reading for more requests
  socket_.async_read_some(boost::asio::buffer(data_, max_length),
                          boost::bind(&session::handle_read, this, _1, _2));
}

void session::handle_write(const boost::system::error_code &error)
{
  if (error)
  {
    BOOST_LOG_TRIVIAL(error) << "Write error: " << error.message();
    safe_delete();
    return;
  }

  // Log successful write for debugging
  // std::cout << "Response sent successfully" << std::endl;

  // Continue reading for more requests (only if we're expecting more)
  if (!socket_.is_open())
  {
    BOOST_LOG_TRIVIAL(info) << "Client closed connection after write";
    safe_delete();
    return;
  }

  socket_.async_read_some(boost::asio::buffer(data_, max_length),
                          boost::bind(&session::handle_read, this,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred));
}