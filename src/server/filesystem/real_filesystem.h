#pragma once

#include "filesystem_interface.h"

class RealFilesystem : public Filesystem {
   public:
    RealFilesystem(const std::string& data_root);

    bool exists(EntityPayload entity, const std::string& id) override;
    std::string read(EntityPayload entity, const std::string& id) override;
    void write(EntityPayload entity, const std::string& id, const std::string& data) override;
    void remove(EntityPayload entity, const std::string& id) override;
    std::vector<std::string> list_ids(EntityPayload entity) override;
    std::string next_id(EntityPayload entity) override;
};
