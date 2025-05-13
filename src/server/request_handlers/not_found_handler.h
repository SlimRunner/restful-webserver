#ifndef NOT_FOUND_HANDLER_H
#define NOT_FOUND_HANDLER_H

#include "request_handler.h"

class NotFoundHandler : public RequestHandler {
   public:
    NotFoundHandler(const std::string& path_prefix, const std::map<std::string, std::string>& args);
    // Always returns a 404 response
    std::unique_ptr<HttpResponse> handle_request(const HttpRequest& request) override;

   private:
    std::string path_prefix_;
};

#endif  // NOT_FOUND_HANDLER_H
