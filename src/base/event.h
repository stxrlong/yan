
#pragma once

#include <memory>
#include <string>

#include "com/logger.h"
#include "type.h"

namespace yan {

/*************************************Event****************************************/
class Event {
public:
    Event() = default;
    ~Event() = default;

    virtual void handle() = 0;

private:
    /* data */
};

using EventPtr = std::shared_ptr<Event>;

/*************************************EventBase****************************************/
template <typename T>
class EventBase : public Event {
public:
    EventBase() = default;
    ~EventBase() = default;

    inline void set_exception(const boost::exception_ptr& e) { pro_.set_exception(e); }
    inline void set_value(T&& value) { pro_.set_value(value); }
    inline void set_value(T const& value) { pro_.set_value(value); }
    inline future<T> get_future() { return pro_.get_future(); }

private:
    promise<T> pro_;
};

}  // namespace yan
