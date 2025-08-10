#include "threadsafe_set.h"

bool ThreadSafeSet::insert(const std::string& value) {
    std::unique_lock lock(mutex_);
    return visited_.insert(value).second;
}

bool ThreadSafeSet::contains(const std::string& value) const {
    std::shared_lock lock(mutex_);
    return visited_.find(value) != visited_.end();
}

void ThreadSafeSet::clear() {
    std::unique_lock lock(mutex_);
    visited_.clear();
}