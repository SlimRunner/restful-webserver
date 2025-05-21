#pragma once

#include <string>
#include <vector>

class Filesystem {
   public:
    virtual ~Filesystem() {}

    virtual bool exists(const std::string& entity, const std::string& id) = 0;
    virtual std::string read(const std::string& entity, const std::string& id) = 0;
    virtual void write(const std::string& entity, const std::string& id,
                       const std::string& data) = 0;
    virtual void remove(const std::string& entity, const std::string& id) = 0;
    virtual std::vector<std::string> list_ids(const std::string& entity) = 0;
    virtual std::string next_id(const std::string& entity) = 0;

   protected:
    void check_id(const std::string& id);
};
