
#include "static_file_handler.h"

#include <boost/log/trivial.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

// Initialize static mime_types_ map
std::map<std::string, std::string> StaticFileHandler::mime_types_ = {
    {".html", "text/html"},      {".css", "text/css"},        {".js", "application/javascript"},
    {".jpg", "image/jpeg"},      {".jpeg", "image/jpeg"},     {".png", "image/png"},
    {".gif", "image/gif"},       {".svg", "image/svg+xml"},   {".txt", "text/plain"},
    {".pdf", "application/pdf"}, {".zip", "application/zip"}, {".json", "application/json"},
    {".xml", "application/xml"}, {".ico", "image/x-icon"}};

StaticFileHandler::StaticFileHandler(const std::string &path_prefix, const std::string &base_dir)
    : path_prefix_(path_prefix), base_dir_(base_dir) {}

HttpResponse StaticFileHandler::HandleRequest(const HttpRequest &request) {
    HttpResponse response;

    // Check if the request method is GET
    if (request.method != "GET") {
        BOOST_LOG_TRIVIAL(warning)
            << "StaticFileHandler rejected non-GET request: " << request.method;
        // Return 400 Bad Request for non-GET methods
        response.status = StatusCode::BAD_REQUEST;
        response.body = "";
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = "0";
        return response;
    }

    // Get the file path relative to the base directory
    std::string relative_path;
    if (request.path.size() > path_prefix_.size()) {
        relative_path = request.path.substr(path_prefix_.size());
    }

    // Make sure the relative path starts with a forward slash
    if (!relative_path.empty() && relative_path[0] != '/') {
        relative_path = "/" + relative_path;
    }

    // Concatenate base directory with relative path to get absolute path
    std::string file_path = base_dir_ + relative_path;

    // Security check - verify the path doesn't escape the base directory
    if (!IsPathSafe(file_path)) {
        BOOST_LOG_TRIVIAL(warning)
            << "Security warning: Attempted path traversal attack: " << file_path;
        response.status = StatusCode::FORBIDDEN;
        response.body = "403 Forbidden";
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = std::to_string(response.body.size());
        return response;
    }

    BOOST_LOG_TRIVIAL(debug) << "StaticFileHandler handling GET request for path: " << file_path;

    // Check if file exists before trying to read it
    if (!std::filesystem::exists(file_path)) {
        BOOST_LOG_TRIVIAL(warning) << "Requested file not found: " << file_path;

        response.status = StatusCode::NOT_FOUND;
        response.body = "404 Not Found";
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = std::to_string(response.body.size());
        return response;
    }

    // Get file size for logging
    std::uintmax_t file_size = std::filesystem::file_size(file_path);
    BOOST_LOG_TRIVIAL(debug) << "File size: " << file_size << " bytes";

    // Read the file
    bool success = false;
    std::string file_content = ReadFile(file_path, success);

    if (success) {
        response.status = StatusCode::OK;
        response.body = file_content;
        response.headers["Content-Type"] = GetMimeType(file_path);
        response.headers["Content-Length"] = std::to_string(file_content.size());

        // For zip files, add Content-Disposition header to help with downloads
        std::string mime_type = GetMimeType(file_path);
        if (mime_type == "application/pdf") {
            // Extract filename from the path
            size_t last_slash = file_path.find_last_of("/");
            std::string filename =
                (last_slash != std::string::npos) ? file_path.substr(last_slash + 1) : file_path;
            response.headers["Content-Disposition"] = "attachment; filename=\"" + filename + "\"";

            // Add cache control headers to prevent caching issues
            response.headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
            response.headers["Pragma"] = "no-cache";
            response.headers["Expires"] = "0";
        } else if (mime_type == "application/zip") {
            // Extract filename from the path
            size_t last_slash = file_path.find_last_of('/');
            std::string filename =
                (last_slash != std::string::npos) ? file_path.substr(last_slash + 1) : file_path;

            response.headers["Content-Disposition"] = "attachment; filename=\"" + filename + "\"";
        }

        BOOST_LOG_TRIVIAL(info) << "Served file: " << file_path << " (" << file_content.size()
                                << " bytes)"
                                << " as " << GetMimeType(file_path);
    } else {
        BOOST_LOG_TRIVIAL(error) << "Failed to read file: " << file_path;
        response.status = StatusCode::NOT_FOUND;
        response.body = "404 Not Found";
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = std::to_string(response.body.size());
    }

    return response;
}

bool StaticFileHandler::CanHandle(const std::string &path) const {
    return path.substr(0, path_prefix_.size()) == path_prefix_;
}

bool StaticFileHandler::IsPathSafe(const std::string &path) const {
    // Convert both paths to absolute, normalized paths
    std::filesystem::path abs_base_dir = std::filesystem::absolute(base_dir_).lexically_normal();
    std::filesystem::path abs_requested_path = std::filesystem::absolute(path).lexically_normal();

    // Convert to strings for comparison
    std::string norm_base_dir = abs_base_dir.string();
    std::string norm_requested_path = abs_requested_path.string();

    // Check if the normalized requested path starts with the normalized base directory
    // This ensures the requested path is within the base directory
    return norm_requested_path.substr(0, norm_base_dir.size()) == norm_base_dir;
}

std::string StaticFileHandler::GetMimeType(const std::string &file_path) const {
    // Get file extension
    size_t last_dot = file_path.find_last_of('.');
    if (last_dot != std::string::npos) {
        std::string extension = file_path.substr(last_dot);

        // Look up MIME type in map
        auto it = mime_types_.find(extension);
        if (it != mime_types_.end()) {
            return it->second;
        }
    }

    // Default MIME type for unknown extensions
    return "application/octet-stream";
}

std::string StaticFileHandler::ReadFile(const std::string &file_path, bool &success) const {
    // Open file in binary mode
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        success = false;
        return "";
    }

    // Get file size by seeking to the end before reaing
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Check if file is too large to handle all at once
    const std::streamsize max_safe_size = 100 * 1024 * 1024;  // 100MB
    if (size > max_safe_size) {
        BOOST_LOG_TRIVIAL(warning)
            << "File too large to serve: " << file_path << " (" << size << " bytes)";
        success = false;
        return "";
    }

    // Use a vector as a buffer for binary data
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        BOOST_LOG_TRIVIAL(error) << "Error reading file: " << file_path;
        BOOST_LOG_TRIVIAL(error) << "Expected size: " << size << ", actual read: " << file.gcount();
        success = false;
        return "";
    }

    success = true;

    // Convert to string - this works for both text and binary files
    return std::string(buffer.data(), size);
}
