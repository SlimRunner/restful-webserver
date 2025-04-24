#ifndef STATIC_FILE_HANDLER_H
#define STATIC_FILE_HANDLER_H

#include "request_handler.h"
#include <map>
#include <string>

class StaticFileHandler : public RequestHandler
{
public:
  StaticFileHandler(const std::string &path_prefix, const std::string &base_dir);

  HttpResponse HandleRequest(const HttpRequest &request) override;
  bool CanHandle(const std::string &path) const override;

private:
  std::string path_prefix_;
  std::string base_dir_;

  // Map of file extensions to MIME types
  static std::map<std::string, std::string> mime_types_;

  // Get the MIME type based on file extension
  std::string GetMimeType(const std::string &file_path) const;

  // Read a file from the filesystem
  std::string ReadFile(const std::string &file_path, bool &success) const;

  // Check if a path is safe (doesn't escape the base directory)
  bool IsPathSafe(const std::string &path) const;
};

#endif // STATIC_FILE_HANDLER_H