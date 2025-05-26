#include "session.h"

#include <boost/bind/bind.hpp>
#include <boost/log/trivial.hpp>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
using namespace boost::placeholders;

static bool isPrefixMatch(const std::string &, const std::string &);

/*
Constructs a new session.
- io_service: Boost I/O context for asynchronous operations.
- handlers: list of request handlers used to process incoming requests.
*/
session::session(boost::asio::io_service &io_service, RoutingMap routes, IHandlerRegistry *registry)
    // adding already_deleted_ member to constructor
    : socket_(io_service), routes_(routes), registry_{registry}, already_deleted_(false) {}

// Returns a reference to the TCP socket associated with this session.
tcp::socket &session::socket() {
    return socket_;
}

// Starts reading from the client asynchronously.
void session::start() {
    BOOST_LOG_TRIVIAL(debug) << "Session started: waiting for request";
    socket_.async_read_some(
        boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this, boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
}

// Directly sets the request buffer (used in unit tests).
void session::set_request(const std::string &req) {
    request_buffer_ = req;
}

/*
Helper function to prevent freeing the same memory twice
as deletion is done in handle_read and handle_write methods
*/
void session::safe_delete() {
    if (!already_deleted_) {
        BOOST_LOG_TRIVIAL(debug) << "Session deleted.";
        already_deleted_ = true;
        delete this;
    }
}

// Parses a raw HTTP request string into a structured HttpRequest object.
HttpRequest session::ParseRequest(const std::string &request_str) {
    HttpRequest request;
    std::istringstream stream(request_str);

    // Parse request line
    stream >> request.method >> request.path >> request.version;

    // Parse headers
    std::string line;
    std::getline(stream, line);  // Consume the rest of the first line

    while (std::getline(stream, line) && line != "\r") {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
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

/*
Handles incoming data on the socket.
Parses one or more complete requests and dispatches them to handlers.
*/
void session::handle_read(const boost::system::error_code &error, size_t bytes_transferred) {
    if (error) {
        BOOST_LOG_TRIVIAL(error) << "Socket read error: " << error.message();
        safe_delete();  // to avoid Double Free Error
        return;
    }

    /*
    Before appending, check if it will exceed a defined maximum buffer size
    buffer size limit defined in session.h, currently 8192 bytes = 8 KB
    */
    if (request_buffer_.size() + bytes_transferred > max_request_size_) {
        BOOST_LOG_TRIVIAL(warning)
            << "Payload too large: " << request_buffer_.size() + bytes_transferred << " bytes";
        std::string error_response =
            "HTTP/1.1 413 Payload Too Large\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n";

        current_response_ = error_response;  // Store the response for logging
        boost::asio::async_write(socket_, boost::asio::buffer(error_response),
                                 boost::bind(&session::handle_write, this, _1));

        return;
    }
    // Accumulate data in request_buffer_
    request_buffer_ += std::string(data_, bytes_transferred);

    while (true) {
        size_t header_end = request_buffer_.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            break;  // wait for full headers
        }

        // Extract headers
        std::string header_part = request_buffer_.substr(0, header_end + 4);

        // Extract content length from headers (temporarily parse it here)
        std::istringstream temp_stream(header_part);
        std::string line;
        size_t content_length = 0;
        while (std::getline(temp_stream, line) && line != "\r") {
            if (line.find("Content-Length:") == 0) {
                std::string value = line.substr(strlen("Content-Length:"));
                while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) value.erase(0, 1);
                content_length = std::stoul(value);
                if (content_length > max_content_length_) {
                    BOOST_LOG_TRIVIAL(warning) << "Content-Length too large: " << content_length;

                    HttpResponse response;
                    response.status = StatusCode::PAYLOAD_TOO_LARGE;
                    response.body = "413 Payload Too Large";
                    response.headers["Content-Type"] = "text/plain";
                    response.headers["Connection"] = "close";
                    response.headers["Content-Length"] = std::to_string(response.body.size());

                    current_response_ = response.ToString();
                    boost::asio::async_write(socket_, boost::asio::buffer(current_response_),
                                             boost::bind(&session::handle_write, this, _1));
                    return;
                }
            }
        }

        // Wait for body to arrive
        size_t total_request_size = header_end + 4 + content_length;
        if (request_buffer_.size() < total_request_size) {
            break;  // wait for full body
        }

        // Extract full request (headers + body)
        std::string full_request = request_buffer_.substr(0, total_request_size);

        // Trim the buffer
        request_buffer_ = request_buffer_.substr(total_request_size);

        // Parse the full request
        HttpRequest request = ParseRequest(full_request);

        // Validate the request (basic sanity check)
        if (request.method.empty() || request.path.empty() || request.version.empty()) {
            try {
                std::string client_ip = socket_.remote_endpoint().address().to_string();
                BOOST_LOG_TRIVIAL(warning) << "Malformed request from " << client_ip << ". Returning 400.";
            } catch (...) {
                BOOST_LOG_TRIVIAL(warning) << "Malformed request from unknown IP. Returning 400.";
            }
            HttpResponse response;
            response.status = StatusCode::BAD_REQUEST;
            response.body = "400 Bad Request\n";
            response.headers["Content-Type"] = "text/plain";
            response.headers["Content-Length"] = std::to_string(response.body.size());
            response.headers["Connection"] = "close";

            current_response_ = response.ToString();

            // [ResponseMetrics] log for malformed request
            std::string client_ip = "[unknown]";
            try {
                client_ip = socket_.remote_endpoint().address().to_string();
            } catch (...) {}
            
            BOOST_LOG_TRIVIAL(info)
                << "[ResponseMetrics] code=400 path=" << request.path
                << " ip=" << client_ip
                << " handler=BadRequest";

            boost::asio::async_write(socket_, boost::asio::buffer(current_response_),
                                    boost::bind(&session::handle_write, this, _1));
            return;
        }

        // Check HTTP version - only accepts HTTP/1.1
        if (request.version != "HTTP/1.1") {
            BOOST_LOG_TRIVIAL(warning) << "Unsupported HTTP version: " << request.version;
            HttpResponse response;
            response.status = StatusCode::BAD_REQUEST;
            response.body = "400 Bad Request\n";
            response.headers["Content-Type"] = "text/plain";
            response.headers["Content-Length"] = std::to_string(response.body.size());
            response.headers["Connection"] = "close";
            current_response_ = response.ToString();

            // [ResponseMetrics] log for invalid version
            std::string client_ip = "[unknown]";
            try {
                client_ip = socket_.remote_endpoint().address().to_string();
            } catch (...) {}
            BOOST_LOG_TRIVIAL(info)
                << "[ResponseMetrics] code=400 path=" << request.path
                << " ip=" << client_ip
                << " handler=BadRequest";

            boost::asio::async_write(socket_, boost::asio::buffer(current_response_),
                                     boost::bind(&session::handle_write, this, _1));
            return;
        }

        try {
            std::string client_ip = socket_.remote_endpoint().address().to_string();
            BOOST_LOG_TRIVIAL(info)
                << "Received " << request.method << " " << request.path << " from " << client_ip;
        } catch (...) {
            BOOST_LOG_TRIVIAL(info)
                << "Received " << request.method << " " << request.path << " from [unknown IP]";
        }
        // Find the appropriate handler for the request path
        std::unique_ptr<HttpResponse> response_ptr = nullptr;
        std::optional<std::string> best_prefix = {};  // empty option

        for (auto &[prefix, _] : routes_) {
            BOOST_LOG_TRIVIAL(debug) << "try " << prefix << " prefix";
            if (isPrefixMatch(prefix, request.path) &&
                (!best_prefix || prefix.size() > best_prefix->size())) {
                best_prefix.emplace(prefix);
            }
        }

        if (best_prefix) {
            BOOST_LOG_TRIVIAL(debug) << "best prefix found " << *best_prefix;
            const auto &registry = *registry_;
            const auto payload = routes_.at(*best_prefix);
            const auto factory = registry.get_factory(payload.handler);
            const auto &handler = factory(*best_prefix, payload.arguments);

            BOOST_LOG_TRIVIAL(debug) << "Handler matched for path: " << request.path
                                     << " using handler type: " << payload.handler;
            response_ptr = handler->handle_request(request);
        } else {
            HttpResponse res;
            BOOST_LOG_TRIVIAL(warning)
                << "No handler for path: " << request.path << ". Sending 404.";

            res.status = StatusCode::NOT_FOUND;
            res.body = "404 Not Found";
            res.headers["Content-Type"] = "text/plain";
            res.headers["Content-Length"] = std::to_string(res.body.size());
            response_ptr = std::make_unique<HttpResponse>(res);
        }

        // Check if the connection should be closed
        bool close_connection = (request.headers.find("Connection") != request.headers.end() &&
                                 request.headers["Connection"] == "close");

        if (close_connection) {
            response_ptr->headers["Connection"] = "close";
        } else {
            response_ptr->headers["Connection"] = "keep-alive";
        }

        // Convert response to string and store it in the member variable
        current_response_ = response_ptr->ToString();
        std::string client_ip = "[unknown]";
        try {
            client_ip = socket_.remote_endpoint().address().to_string();
        } catch (...) {}

        // Safely determine the name of the matched request handler.
        // If a best-matching route prefix was found, look it up in the routes_ map
        // and extract the handler name. Fallback to "NotFoundHandler" if no match exists or route lookup fails.
        std::string handler_name = "NotFoundHandler";
        if (best_prefix) {
            auto it = routes_.find(*best_prefix);
            if (it != routes_.end()) {
                handler_name = it->second.handler;
            }
        }

        BOOST_LOG_TRIVIAL(info)
            << "[ResponseMetrics] code=" << static_cast<int>(response_ptr->status)
            << " path=" << request.path
            << " ip=" << client_ip
            << " handler=" << handler_name;
        boost::asio::async_write(socket_, boost::asio::buffer(current_response_),
                                 boost::bind(&session::handle_write, this, _1));

        if (close_connection) {
            return;  // Don't read anymore
        }
    }

    // Continue reading for more requests
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
                            boost::bind(&session::handle_read, this, _1, _2));
}

/*
Called after writing a response to the client.
If the connection is still open, continues reading the next request.
*/
void session::handle_write(const boost::system::error_code &error) {
    if (error) {
        BOOST_LOG_TRIVIAL(error) << "Write error: " << error.message();
        safe_delete();
        return;
    }

    // Continue reading for more requests (only if we're expecting more)
    if (!socket_.is_open()) {
        BOOST_LOG_TRIVIAL(info) << "Client closed connection after write";
        safe_delete();
        return;
    }

    socket_.async_read_some(
        boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this, boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
}

static bool isPrefixMatch(const std::string &handler_prefix, const std::string &request_path) {
    // match if entire path is '/'
    if (handler_prefix == "/") return true;
    if (request_path.size() < handler_prefix.size()) return false;
    if (request_path.compare(0, handler_prefix.size(), handler_prefix) != 0) return false;

    // if they don't match path is longer so it should have a trailing '/'
    return handler_prefix.size() == request_path.size() ||
           request_path[handler_prefix.size()] == '/';
}
