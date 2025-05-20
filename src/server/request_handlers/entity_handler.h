#ifndef ENTITY_HANDLER_H
#define ENTITY_HANDLER_H

#include "request_handler.h"
#include "real_filesystem.h"
#include "file_io_exception.h"
#include "not_found_exception.h"

class EntityHandler : public RequestHandler {
   public:
    EntityHandler(const std::string& path_prefix, const std::map<std::string, std::string>& args);

    void set_filesystem(std::unique_ptr<Filesystem> fs);
    Filesystem* get_filesystem() const;

    std::unique_ptr<HttpResponse> handle_request(const HttpRequest& request) override;

   protected:
    std::unique_ptr<Filesystem> fs_;


   private:
    std::string path_prefix_;
    std::string base_dir_;
};

#endif  // ENTITY_HANDLER_H
