#include "filesystem_interface.h"

#include <boost/log/trivial.hpp>

#include "invalid_id_exception.h"

void Filesystem::check_id(const std::string& id) {
    try {
        BOOST_LOG_TRIVIAL(debug) << "Checking id: " << id;

        int numeric_id = std::stoi(id);

        if (numeric_id <= 0) {
            BOOST_LOG_TRIVIAL(warning) << "Invalid id (" << id << "): ID must be a positive integer";

            throw InvalidIdException("Invalid id (" + id + "): ID must be a positive integer");
        }
    } catch (...) {
        BOOST_LOG_TRIVIAL(warning) << "Invalid id (" << id << "): ID must be a positive integer";

        throw InvalidIdException("Invalid id (" + id + "): ID must be a positive integer");
    }
    
    BOOST_LOG_TRIVIAL(debug) << "Id is valid: " << id;
}
