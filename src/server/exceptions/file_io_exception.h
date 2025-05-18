#pragma once
#include <stdexcept>

class FileIOException : public std::runtime_error {
public:
    explicit FileIOException(const std::string& msg)
        : std::runtime_error(msg) {}
};
