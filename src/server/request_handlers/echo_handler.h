#ifndef ECHO_HANDLER_H
#define ECHO_HANDLER_H

#include "request_handler.h"

class EchoHandler : public RequestHandler
{
public:
    EchoHandler(const std::string &path_prefix);

    HttpResponse HandleRequest(const HttpRequest &request) override;
    bool CanHandle(const std::string &path) const override;

private:
    std::string path_prefix_;
};

#endif // ECHO_HANDLER_H