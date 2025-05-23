#pragma once

#include <optional>
#include <string>
#include <vector>

struct EntityPayload {
    std::string path;
    std::string name;

    std::string make_path();
    std::string make_path(const std::string& id);
    std::string make_name(const std::string& id);
};

class Filesystem {
   public:
    virtual ~Filesystem() {}

    virtual bool exists(EntityPayload entity, const std::string& id) = 0;
    virtual std::string read(EntityPayload entity, const std::string& id) = 0;
    virtual void write(EntityPayload entity, const std::string& id, const std::string& data) = 0;
    virtual void remove(EntityPayload entity, const std::string& id) = 0;
    virtual std::vector<std::string> list_ids(EntityPayload entity) = 0;
    virtual std::string next_id(EntityPayload entity) = 0;

   protected:
    void check_id(const std::string& id);
};
