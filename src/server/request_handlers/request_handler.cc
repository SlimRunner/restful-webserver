#include "request_handler.h"
#include <sstream>

std::string HttpResponse::ToString() const
{
    std::stringstream ss;

    // Status line
    ss << "HTTP/1.1 ";
    switch (status)
    {
    case StatusCode::OK:
        ss << "200 OK";
        break;
    case StatusCode::BAD_REQUEST:
        ss << "400 Bad Request";
        break;
    case StatusCode::NOT_FOUND:
        ss << "404 Not Found";
        break;
    case StatusCode::INTERNAL_SERVER_ERROR:
        ss << "500 Internal Server Error";
        break;
    default:
        ss << "200 OK"; // Default to 200 OK
    }
    ss << "\r\n";

    // Headers
    for (const auto &header : headers)
    {
        ss << header.first << ": " << header.second << "\r\n";
    }

    // Empty line separating headers from body
    ss << "\r\n";

    // Body
    ss << body;

    return ss.str();
}