#pragma once
#include <stdexcept>

class NotFoundException : public std::runtime_error {
public:
    explicit NotFoundException(const std::string& msg)
        : std::runtime_error(msg) {}
};
