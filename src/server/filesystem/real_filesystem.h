#pragma once

#include "filesystem_interface.h"

class RealFilesystem : public Filesystem {
   public:
    RealFilesystem(const std::string& data_root);

    bool exists(const std::string& entity, const std::string& id) override;
    std::string read(const std::string& entity, const std::string& id) override;
    void write(const std::string& entity, const std::string& id, const std::string& data) override;
    void remove(const std::string& entity, const std::string& id) override;
    std::vector<std::string> list_ids(const std::string& entity) override;
    std::string next_id(const std::string& entity) override;

   private:
    std::string data_path_;

    std::string make_path(const std::string& entity, const std::string& id = "") const;
};
