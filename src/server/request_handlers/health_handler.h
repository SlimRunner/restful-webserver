#ifndef HEALTH_HANDLER_H
#define HEALTH_HANDLER_H

#include "request_handler.h"

// Responds with 200 OK and "OK" body for health checks
class HealthHandler : public RequestHandler {
 public:
  HealthHandler(const std::string& path_prefix, const std::map<std::string, std::string>& args);

  std::unique_ptr<HttpResponse> handle_request(const HttpRequest& request) override;

 private:
  std::string path_prefix_;
};

#endif  // HEALTH_HANDLER_H