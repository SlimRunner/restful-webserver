#ifndef ECHO_HANDLER_H
#define ECHO_HANDLER_H

#include "request_handler.h"

class EchoHandler : public RequestHandler {
   public:
    EchoHandler(const std::string &path_prefix, const std::map<std::string, std::string>& args);

    std::shared_ptr<HttpResponse> handle_request(const HttpRequest& request) override;
    bool can_handle(const std::string& path) const override {
        return path == path_prefix_ || 
           (path.size() > path_prefix_.size() &&
            path.compare(0, path_prefix_.size(), path_prefix_) == 0 &&
            path[path_prefix_.size()] == '/'); 
    }

   private:
    std::string path_prefix_;
};

#endif  // ECHO_HANDLER_H
