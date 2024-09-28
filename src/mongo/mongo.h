#pragma once

#include "base/event_ops.h"
#include "base/type.h"
#include "mongo_event.h"

namespace yan {
namespace mongo {

class MongoDB {
public:
    MongoDB() = default;
    ~MongoDB() = default;

    int init(const std::string &uri) {
        operators_ = std::make_shared<EventOps>("mongodb", 1);

        context_ = std::make_shared<MongoContext>();
        return context_->init(uri);
    }

    template <typename... Args>
    inline future<int64_t> write(Args &&...args) {
        return create_event<int64_t, WriteEvent>(std::forward<Args>(args)...);
    }

    template <typename Ret, typename... Args>
    inline future<Ret> read(Args &&...args) {
        return create_event<Ret, ReadEvent<Ret>>(std::forward<Args>(args)...);
    }

    template <typename Args>
    inline future<int64_t> del(Args &&args) {
        return create_event<int64_t, DeleteEvent>(std::forward<Args>(args));
    }

private:
    template <typename Ret, typename MongoEvent, typename... Args>
    inline future<Ret> create_event(Args &&...args) {
        auto event = std::make_shared<MongoEvent>(context_);
        try {
            event->prehandle(std::forward<Args>(args)...);
            assert(operators_);
            operators_->insert(event);
        } catch (const std::exception &e) {
            return make_yan_exception_future<Ret>(std::runtime_error(e.what()));
        }

        return event->get_future();
    }

private:
    EventOpsPtr operators_ = nullptr;
    MongoContextPtr context_ = nullptr;
};

}  // namespace mongo
}  // namespace yan
