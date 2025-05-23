#pragma once

#include <map>

#include "filesystem_interface.h"

class MockFilesystem : public Filesystem {
   public:
    bool exists(EntityPayload entity, const std::string& id) override;
    std::string read(EntityPayload entity, const std::string& id) override;
    void write(EntityPayload entity, const std::string& id, const std::string& data) override;
    void remove(EntityPayload entity, const std::string& id) override;
    std::vector<std::string> list_ids(EntityPayload entity) override;
    std::string next_id(EntityPayload entity) override;

    void reset();

   private:
    std::map<std::string, std::map<std::string, std::string>> data_;
};
