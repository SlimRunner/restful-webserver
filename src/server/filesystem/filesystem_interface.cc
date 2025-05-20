#include "filesystem_interface.h"

#include <boost/log/trivial.hpp>

#include "tagged_exceptions.h"

void Filesystem::check_id(const std::string& id) {
    try {
        BOOST_LOG_TRIVIAL(debug) << "Checking id: " << id;

        int numeric_id = std::stoi(id);

        if (numeric_id <= 0) {
            BOOST_LOG_TRIVIAL(warning)
                << "Invalid id (" << id << "): ID must be a positive integer";

            throw expt::invalid_id_exception("Invalid id (" + id +
                                             "): ID must be a positive integer");
        }
    } catch (...) {
        BOOST_LOG_TRIVIAL(warning) << "Invalid id (" << id << "): ID must be a positive integer";

        throw expt::invalid_id_exception("Invalid id (" + id + "): ID must be a positive integer");
    }

    BOOST_LOG_TRIVIAL(debug) << "Id is valid: " << id;
}
