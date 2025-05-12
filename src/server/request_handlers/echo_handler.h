#ifndef ECHO_HANDLER_H
#define ECHO_HANDLER_H

#include "request_handler.h"

class EchoHandler : public RequestHandler {
   public:
    EchoHandler(const std::string& path_prefix, const std::map<std::string, std::string>& args);

    std::shared_ptr<HttpResponse> handle_request(const HttpRequest& request) override;

   private:
    std::string path_prefix_;
};

#endif  // ECHO_HANDLER_H
