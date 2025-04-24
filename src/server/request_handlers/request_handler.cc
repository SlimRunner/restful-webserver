#include "request_handler.h"
#include <sstream>

std::string HttpResponse::ToString() const
{
    std::string headers_str;

    // Status line
    headers_str += "HTTP/1.1 ";
    switch (status)
    {
    case StatusCode::OK:
        headers_str += "200 OK";
        break;
    case StatusCode::BAD_REQUEST:
        headers_str += "400 Bad Request";
        break;
    case StatusCode::FORBIDDEN:
        headers_str += "403 Forbidden";
        break;
    case StatusCode::NOT_FOUND:
        headers_str += "404 Not Found";
        break;
    case StatusCode::INTERNAL_SERVER_ERROR:
        headers_str += "500 Internal Server Error";
        break;
    default:
        headers_str += "500 Internal Server Error";
    }
    headers_str += "\r\n";

    // Headers
    for (const auto &header : headers)
    {
        headers_str += header.first + ": " + header.second + "\r\n";
    }

    // Empty line separating headers from body
    headers_str += "\r\n";

    // Create the full response by concatenating headers and body
    // Using string's append to handle binary data properly
    std::string full_response;
    full_response.reserve(headers_str.size() + body.size());
    full_response.append(headers_str);
    full_response.append(body);

    return full_response;
}