#include "real_filesystem.h"

#include <algorithm>
#include <boost/log/trivial.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "tagged_exceptions.h"

namespace fs = std::filesystem;

RealFilesystem::RealFilesystem(const std::string& data_path) {
    fs::create_directories(data_path);
}

bool RealFilesystem::exists(EntityPayload entity, const std::string& id) {
    return fs::exists(entity.make_path(id));
}

std::string RealFilesystem::read(EntityPayload entity, const std::string& id) {
    if (!fs::exists(entity.make_path(id))) {
        BOOST_LOG_TRIVIAL(warning) << "No such entity or ID: " << entity.make_name(id);

        throw expt::not_found_exception("No such entity or ID: " + entity.make_name(id));
    }

    std::string path = entity.make_path(id);
    std::ifstream file(path);
    if (!file.is_open()) {
        BOOST_LOG_TRIVIAL(warning) << "Could not open file for reading: " << path;

        throw expt::file_io_exception("Could not open file for reading: " + path);
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void RealFilesystem::write(EntityPayload entity, const std::string& id, const std::string& data) {
    check_id(id);

    std::string dir = entity.make_path();
    fs::create_directories(dir);

    std::string path = entity.make_path(id);

    std::ofstream file(path);
    if (!file.is_open()) {
        BOOST_LOG_TRIVIAL(warning) << "Could not open file for writing: " << path;

        throw expt::file_io_exception("Could not open file for writing: " + path);
    }

    BOOST_LOG_TRIVIAL(info) << "Writing " << data << " to " << path;

    file << data;
}

void RealFilesystem::remove(EntityPayload entity, const std::string& id) {
    std::string path = entity.make_path(id);

    BOOST_LOG_TRIVIAL(info) << "Removing " << path;

    if (!fs::remove(path)) {
        BOOST_LOG_TRIVIAL(warning) << "Could not remove file: " << path;

        throw expt::not_found_exception("Could not remove file: " + path);
    }
}

std::vector<std::string> RealFilesystem::list_ids(EntityPayload entity) {
    std::vector<std::string> ids;
    std::string dir_path = entity.make_path();

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

std::string RealFilesystem::next_id(EntityPayload entity) {
    auto ids = list_ids(entity);
    int max_id = 0;

    for (const auto& id_str : ids) {
        try {
            int id = std::stoi(id_str);
            if (id > max_id) {
                max_id = id;
            }
        } catch (const std::invalid_argument&) {
            // Non-numeric ID, ignore
        } catch (const std::out_of_range&) {
            // Too big to be an int, ignore
        }
    }

    return std::to_string(max_id + 1);
}
