#include "../request_handlers/request_handler.h"

#include <sstream>

#include "gtest/gtest.h"

// test that the handler status default: 500 Internal Server Error
TEST(RequestHandlerStatus, DefaultRequest) {
    HttpResponse httpRes = {};
    std::string res = httpRes.ToString();
    EXPECT_EQ(res, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
}

// test that the handler status default: 200 OK
TEST(RequestHandlerStatus, Request200Empty) {
    HttpResponse httpRes = {StatusCode::OK, {}, ""};
    std::string res = httpRes.ToString();
    EXPECT_EQ(res, "HTTP/1.1 200 OK\r\n\r\n");
}

// test that the handler status default: 400 Bad Request
TEST(RequestHandlerStatus, Request400Empty) {
    HttpResponse httpRes = {StatusCode::BAD_REQUEST, {}, ""};
    std::string res = httpRes.ToString();
    EXPECT_EQ(res, "HTTP/1.1 400 Bad Request\r\n\r\n");
}

// test that the handler status default: 403 Forbidden
TEST(RequestHandlerStatus, Request403Empty) {
    HttpResponse httpRes = {StatusCode::FORBIDDEN, {}, ""};
    std::string res = httpRes.ToString();
    EXPECT_EQ(res, "HTTP/1.1 403 Forbidden\r\n\r\n");
}

// test that the handler status default: 404 Not Found
TEST(RequestHandlerStatus, Request404Empty) {
    HttpResponse httpRes = {StatusCode::NOT_FOUND, {}, ""};
    std::string res = httpRes.ToString();
    EXPECT_EQ(res, "HTTP/1.1 404 Not Found\r\n\r\n");
}

// test that the handler status default: 500 Internal Server Error
TEST(RequestHandlerStatus, Request500Empty) {
    HttpResponse httpRes = {StatusCode::INTERNAL_SERVER_ERROR, {}, ""};
    std::string res = httpRes.ToString();
    EXPECT_EQ(res, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
}

// test that the body and HTTP header matches verbatim
TEST(RequestHandlerPayload, RequestWithBody) {
    std::stringstream body;
    body << "Lorem Ipsum is simply dummy text of the printing and\n"
         << "typesetting industry. Lorem Ipsum has been the industry's\n"
         << "standard dummy text ever since the 1500s, when an unknown\n"
         << "printer took a galley of type and scrambled it to make a\n"
         << "type specimen book. It has survived not only five\n"
         << "centuries, but also the leap into electronic typesetting,\n"
         << "remaining essentially unchanged. It was popularised in...";
    HttpResponse httpRes = {StatusCode::OK, {}, body.str()};
    const std::string header = "HTTP/1.1 200 OK\r\n\r\n";

    std::string res = httpRes.ToString();
    EXPECT_EQ(res, header + body.str());
}

// test that each header is present in the response. The request does
// not preserve order, so that cannot be tested.
TEST(RequestHandlerPayload, RequestWithHeader) {
    const std::map<std::string, std::string> headers = {
        {"Content-Type", "text/html"}, {"Content-Length", "123"},    {"Connection", "keep-alive"},
        {"Cache-Control", "no-cache"}, {"Server", "TestServer/1.0"},
    };

    HttpResponse httpRes = {StatusCode::OK, headers, ""};
    // NOTE: the headers are sorted because of the underlying data
    // structure (map -> balanced tree)
    const std::string header =
        "HTTP/1.1 200 OK\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: keep-alive\r\n"
        "Content-Length: 123\r\n"
        "Content-Type: text/html\r\n"
        "Server: TestServer/1.0\r\n\r\n";
    //--
    std::string res = httpRes.ToString();

    EXPECT_EQ(res, header);
}
