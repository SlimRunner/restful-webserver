#pragma once
#include <stdexcept>

class InvalidIdException : public std::runtime_error {
public:
    explicit InvalidIdException(const std::string& msg)
        : std::runtime_error(msg) {}
};
