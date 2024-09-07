

#pragma once

#include "com/thread_pool.h"
#include "event.h"

namespace yan {

class EventOps : public ThreadPool {
    using Base = ThreadPool;

public:
    EventOps(const std::string &dbname, const int size) : Base(dbname, size) {}
    ~EventOps() = default;

    inline bool insert(const EventPtr &event) {
        return Base::insert([event]() { event->handle(); });
    }
};

using EventOpsPtr = std::shared_ptr<EventOps>;

}  // namespace yan
