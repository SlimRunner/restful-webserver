#include "mock_filesystem.h"

#include <algorithm>
#include <boost/log/trivial.hpp>
#include <stdexcept>

#include "tagged_exceptions.h"

bool MockFilesystem::exists(EntityPayload entity, const std::string& id) {
    if (!data_.count(entity.name)) {
        return false;
    }
    return data_[entity.name].count(id);
}

std::string MockFilesystem::read(EntityPayload entity, const std::string& id) {
    if (!exists(entity, id)) {
        BOOST_LOG_TRIVIAL(warning)
            << "MockFilesystem: No such entity or ID: " << entity.make_name(id);

        throw expt::not_found_exception("MockFilesystem: No such entity or ID: " +
                                        entity.make_name(id));
    }
    return data_[entity.name][id];
}

void MockFilesystem::write(EntityPayload entity, const std::string& id, const std::string& data) {
    check_id(id);

    BOOST_LOG_TRIVIAL(debug) << "MockFilesystem: Writing to " << entity.make_name(id);

    data_[entity.name][id] = data;
}

void MockFilesystem::remove(EntityPayload entity, const std::string& id) {
    if (!exists(entity, id)) {
        BOOST_LOG_TRIVIAL(warning)
            << "MockFilesystem: Could not remove file: " << entity.make_name(id);

        throw expt::not_found_exception("MockFilesystem: Could not remove file: " +
                                        entity.make_name(id));
    }

    BOOST_LOG_TRIVIAL(debug) << "MockFilesystem: Removing " << entity.make_name(id);

    data_[entity.name].erase(id);
}

std::vector<std::string> MockFilesystem::list_ids(EntityPayload entity) {
    std::vector<std::string> ids;
    if (data_.count(entity.name)) {
        for (const auto& [id, _] : data_[entity.name]) {
            ids.push_back(id);
        }
    }
    return ids;
}

std::string MockFilesystem::next_id(EntityPayload entity) {
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
