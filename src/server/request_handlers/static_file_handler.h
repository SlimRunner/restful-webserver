#ifndef STATIC_FILE_HANDLER_H
#define STATIC_FILE_HANDLER_H

#include <map>
#include <string>

#include "request_handler.h"

class StaticFileHandler : public RequestHandler {
   public:
    StaticFileHandler(const std::string &path_prefix, const std::map<std::string, std::string>& args);

    std::shared_ptr<HttpResponse> handle_request(const HttpRequest &request) override;
    bool can_handle(const std::string& path) const override;
    std::string get_prefix() const override;
   private:
    std::string path_prefix_;
    std::string base_dir_;

    // Map of file extensions to MIME types
    static std::map<std::string, std::string> mime_types_;

    // Get the MIME type based on file extension
    std::string get_mime_type(const std::string &file_path) const;

    // Read a file from the filesystem
    std::string read_file(const std::string &file_path, bool &success) const;

    // Check if a path is safe (doesn't escape the base directory)
    bool is_path_safe(const std::string &path) const;
};

#endif  // STATIC_FILE_HANDLER_H
