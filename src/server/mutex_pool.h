#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "tagged_exceptions.h"

template <typename Key>
class MutexPool {
   public:
    using MutexPtr = std::shared_ptr<std::mutex>;

    MutexPool() : mutex_pool_() {}

    void insert(const Key& key) {
        std::lock_guard<std::mutex> lock(map_mutex_);
        if (!mutex_pool_.count(key)) {
            mutex_pool_.emplace(key, std::make_shared<std::mutex>());
            mutex_count_.emplace(key, 1);
        } else {
            mutex_count_.at(key) += 1;
        }
    }

    MutexPtr request(const Key& key) {
        std::lock_guard<std::mutex> lock(map_mutex_);
        validate_key(key);
        return mutex_pool_.at(key);
    }

    void erase(const Key& key) {
        std::lock_guard<std::mutex> lock(map_mutex_);
        validate_key(key);
        mutex_count_.at(key) -= 1;
        if (mutex_count_.at(key) <= 0) {
            mutex_count_.erase(key);
            mutex_pool_.erase(key);
        }
    }

   private:
    mutable std::mutex map_mutex_;
    std::map<Key, MutexPtr> mutex_pool_;
    std::map<Key, int> mutex_count_;

    void validate_key(const Key& key) {
        if (!mutex_pool_.count(key)) {
            std::string msg = "mutex missing at key: " + key;
            throw expt::mutex_missing_exception(msg);
        }
    }
};
