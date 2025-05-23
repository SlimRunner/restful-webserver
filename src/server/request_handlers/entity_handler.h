#ifndef ENTITY_HANDLER_H
#define ENTITY_HANDLER_H

#include "mutex_pool.h"
#include "real_filesystem.h"
#include "request_handler.h"

class EntityHandler : public RequestHandler {
   public:
    EntityHandler(const std::string& path_prefix, const std::map<std::string, std::string>& args);
    ~EntityHandler() override;

    void set_filesystem(std::unique_ptr<Filesystem> fs);
    Filesystem* get_filesystem() const;

    std::unique_ptr<HttpResponse> handle_request(const HttpRequest& request) override;

   protected:
    std::unique_ptr<Filesystem> fs_;

   private:
    static MutexPool<std::string> mutex_map_;
    std::string path_prefix_;
    std::string base_dir_;
};

#endif  // ENTITY_HANDLER_H
