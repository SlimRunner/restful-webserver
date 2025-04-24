// request_handler.h
#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <string>
#include <map>

// HTTP status codes
enum class StatusCode
{
    OK = 200,
    BAD_REQUEST = 400,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500
};

// HTTP request structure
struct HttpRequest
{
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

// HTTP response structure
struct HttpResponse
{
    StatusCode status;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string ToString() const;
};

// Base request handler interface
class RequestHandler
{
public:
    virtual ~RequestHandler() = default;

    // Handle a request and generate a response
    virtual HttpResponse HandleRequest(const HttpRequest &request) = 0;

    // Check if this handler should process the given path
    virtual bool CanHandle(const std::string &path) const = 0;
};

#endif // REQUEST_HANDLER_H