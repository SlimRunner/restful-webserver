#include "mock_filesystem.h"

#include <algorithm>
#include <boost/log/trivial.hpp>
#include <stdexcept>

#include "tagged_exceptions.h"

bool MockFilesystem::exists(const std::string& entity, const std::string& id) {
    if (!data_.count(entity)) {
        return false;
    }
    return data_[entity].count(id);
}

std::string MockFilesystem::read(const std::string& entity, const std::string& id) {
    if (!exists(entity, id)) {
        BOOST_LOG_TRIVIAL(warning)
            << "MockFilesystem: No such entity or ID: " << entity << "/" << id;

        throw expt::not_found_exception("MockFilesystem: No such entity or ID: " + entity + "/" +
                                        id);
    }
    return data_[entity][id];
}

void MockFilesystem::write(const std::string& entity, const std::string& id,
                           const std::string& data) {
    check_id(id);

    BOOST_LOG_TRIVIAL(debug) << "MockFilesystem: Writing to " << entity << "/" << id;

    data_[entity][id] = data;
}

void MockFilesystem::remove(const std::string& entity, const std::string& id) {
    if (!exists(entity, id)) {
        BOOST_LOG_TRIVIAL(warning)
            << "MockFilesystem: Could not remove file: " << entity << "/" << id;

        throw expt::not_found_exception("MockFilesystem: Could not remove file: " + entity + "/" +
                                        id);
    }

    BOOST_LOG_TRIVIAL(debug) << "MockFilesystem: Removing " << entity << "/" << id;

    data_[entity].erase(id);
}

std::vector<std::string> MockFilesystem::list_ids(const std::string& entity) {
    std::vector<std::string> ids;
    if (data_.count(entity)) {
        for (const auto& [id, _] : data_[entity]) {
            ids.push_back(id);
        }
    }
    return ids;
}

std::string MockFilesystem::next_id(const std::string& entity) {
    auto ids = list_ids(entity);
    int max_id = 0;

    if (!ids.empty()) {
        auto max_element_it = std::max_element(ids.begin(), ids.end());
        max_id = std::stoi(*max_element_it);
    }

    return std::to_string(max_id + 1);
}

void MockFilesystem::reset() {
    BOOST_LOG_TRIVIAL(debug) << "MockFilesystem: Resetting";

    data_.clear();
}
