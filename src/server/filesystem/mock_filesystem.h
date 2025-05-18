#pragma once

#include <map>

#include "filesystem_interface.h"

class MockFilesystem : public Filesystem {
public:
    bool exists(const std::string& entity, const std::string& id) override;
    std::string read(const std::string& entity, const std::string& id) override;
    void write(const std::string& entity, const std::string& id, const std::string& data) override;
    void remove(const std::string& entity, const std::string& id) override;
    std::vector<std::string> list_ids(const std::string& entity) override;
    std::string next_id(const std::string& entity) override;

    void reset();

private:
    std::map<std::string, std::map<std::string, std::string>> data_;
};
