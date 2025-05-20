#include "real_filesystem.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <boost/log/trivial.hpp>

#include "file_io_exception.h"
#include "not_found_exception.h"

namespace fs = std::filesystem;

RealFilesystem::RealFilesystem(const std::string& data_path)
    : data_path_(data_path) {
    fs::create_directories(data_path);
}

std::string RealFilesystem::make_path(const std::string& entity, const std::string& id) const {
    if (id.empty()) {
        return data_path_ + "/" + entity;
    }
    return data_path_ + "/" + entity + "/" + id;
}

bool RealFilesystem::exists(const std::string& entity, const std::string& id) {
    return fs::exists(make_path(entity, id));
}

std::string RealFilesystem::read(const std::string& entity, const std::string& id) {
    if (!exists(entity, id)) {
        BOOST_LOG_TRIVIAL(warning) << "No such entity or ID: " << entity << "/" << id;

        // TODO: please catch this in the api request handler and return 404 http response
        throw NotFoundException("No such entity or ID: " + entity + "/" + id);
    }

    std::string path = make_path(entity, id);
    std::ifstream file(path);
    if (!file.is_open()) {
        BOOST_LOG_TRIVIAL(warning) << "Could not open file for reading: " << path;

        // TODO: please catch this in the api request handler and return 500 http response
        throw FileIOException("Could not open file for reading: " + path);
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void RealFilesystem::write(const std::string& entity, const std::string& id, const std::string& data) {
    check_id(id);

    std::string dir = make_path(entity);
    fs::create_directories(dir);

    std::string path = make_path(entity, id);
    std::ofstream file(path);
    if (!file.is_open()) {
        BOOST_LOG_TRIVIAL(warning) << "Could not open file for writing: " << path;

        // TODO: please catch this in the api request handler and return 500 http response
        throw FileIOException("Could not open file for writing: " + path);
    }

    BOOST_LOG_TRIVIAL(info) << "Writing " << data << " to " << path;

    file << data;
}

void RealFilesystem::remove(const std::string& entity, const std::string& id) {
    std::string path = make_path(entity, id);

    BOOST_LOG_TRIVIAL(info) << "Removing " << path;

    if (!fs::remove(path)) {
        BOOST_LOG_TRIVIAL(warning) << "Could not remove file: " << path;

        // TODO: please catch this in the api request handler and return 404 http response
        throw NotFoundException("Could not remove file: " + path);
    }
}

std::vector<std::string> RealFilesystem::list_ids(const std::string& entity) {
    std::vector<std::string> ids;
    std::string dir_path = make_path(entity);

    if (!fs::exists(dir_path)) {
        return ids;
    }

    for (const auto& entry : fs::directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            ids.push_back(entry.path().filename().string());
        }
    }
    return ids;
}

std::string RealFilesystem::next_id(const std::string& entity) {
    auto ids = list_ids(entity);
    int max_id = 0;

    if (!ids.empty()) {
        auto max_element_it = std::max_element(ids.begin(), ids.end());
        max_id = std::stoi(*max_element_it);
    }

    return std::to_string(max_id + 1);
}
