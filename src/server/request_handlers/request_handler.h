// request_handler.h
#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <map>
#include <memory>
#include <ostream>
#include <string>

// HTTP status codes
enum class StatusCode {
    OK = 200,
    BAD_REQUEST = 400,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500,

};

// HTTP request structure
struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

// HTTP response structure
struct HttpResponse {
    StatusCode status;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string ToString() const;
};

// Base request handler interface
class RequestHandler {
   public:
    virtual ~RequestHandler() = default;

    // Handle a request and generate a response
    virtual std::unique_ptr<HttpResponse> handle_request(const HttpRequest& request) = 0;
};

// Overload the stream insertion operator for StatusCode
// This allows us to log or print StatusCode values in a human-readable format,
// such as "200 OK" or "404 Not Found", instead of just the raw enum integer.
// It's especially useful for Boost.Log or std::cout/debugging purposes.
inline std::ostream& operator<<(std::ostream& os, StatusCode code) {
    switch (code) {
        case StatusCode::OK:
            return os << "200 OK";
        case StatusCode::BAD_REQUEST:
            return os << "400 Bad Request";
        case StatusCode::FORBIDDEN:
            return os << "403 Forbidden";
        case StatusCode::NOT_FOUND:
            return os << "404 Not Found";
        case StatusCode::INTERNAL_SERVER_ERROR:
            return os << "500 Internal Server Error";
        default:
            return os << "Unknown Status (" << static_cast<int>(code) << ")";
    }
}
#endif  // REQUEST_HANDLER_H
