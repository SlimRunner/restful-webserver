#ifndef SLEEP_HANDLER_H
#define SLEEP_HANDLER_H

#include "request_handler.h"

class SleepHandler : public RequestHandler {
   public:
    SleepHandler(const std::string& path_prefix, const std::map<std::string, std::string>& args);

    std::unique_ptr<HttpResponse> handle_request(const HttpRequest& request) override;

   private:
    std::string path_prefix_;
    size_t delay_in_millis_;
};

#endif  // SLEEP_HANDLER_H
