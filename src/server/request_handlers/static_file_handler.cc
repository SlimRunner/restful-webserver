#include "static_file_handler.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

// Initialize static mime_types_ map
std::map<std::string, std::string> StaticFileHandler::mime_types_ = {
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".txt", "text/plain"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".json", "application/json"},
    {".xml", "application/xml"},
    {".ico", "image/x-icon"}};

StaticFileHandler::StaticFileHandler(const std::string &path_prefix, const std::string &base_dir)
    : path_prefix_(path_prefix), base_dir_(base_dir) {}

HttpResponse StaticFileHandler::HandleRequest(const HttpRequest &request)
{
    HttpResponse response;

    // Get the file path relative to the base directory
    std::string relative_path;
    if (request.path.size() > path_prefix_.size())
    {
        relative_path = request.path.substr(path_prefix_.size());
    }

    // Make sure the relative path starts with a forward slash
    if (!relative_path.empty() && relative_path[0] != '/')
    {
        relative_path = "/" + relative_path;
    }

    // Concatenate base directory with relative path to get absolute path
    std::string file_path = base_dir_ + relative_path;

    // Read the file
    bool success = false;
    std::string file_content = ReadFile(file_path, success);

    if (success)
    {
        response.status = StatusCode::OK;
        response.body = file_content;
        response.headers["Content-Type"] = GetMimeType(file_path);
        response.headers["Content-Length"] = std::to_string(file_content.size());
    }
    else
    {
        response.status = StatusCode::NOT_FOUND;
        response.body = "404 Not Found";
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = std::to_string(response.body.size());
    }

    return response;
}

bool StaticFileHandler::CanHandle(const std::string &path) const
{
    return path.substr(0, path_prefix_.size()) == path_prefix_;
}

std::string StaticFileHandler::GetMimeType(const std::string &file_path) const
{
    // Get file extension
    size_t last_dot = file_path.find_last_of('.');
    if (last_dot != std::string::npos)
    {
        std::string extension = file_path.substr(last_dot);

        // Look up MIME type in map
        auto it = mime_types_.find(extension);
        if (it != mime_types_.end())
        {
            return it->second;
        }
    }

    // Default MIME type for unknown extensions
    return "application/octet-stream";
}

std::string StaticFileHandler::ReadFile(const std::string &file_path, bool &success) const
{
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open())
    {
        success = false;
        return "";
    }

    // Read file content
    std::ostringstream content;
    content << file.rdbuf();
    success = true;
    return content.str();
}