#pragma once
#include <unordered_set>
#include <shared_mutex>
#include <string>

class ThreadSafeSet {
public:
    bool insert(const std::string& value);
    bool contains(const std::string& value) const;
    void clear();

private:
    mutable std::shared_mutex mutex_;
    std::unordered_set<std::string> visited_;
};