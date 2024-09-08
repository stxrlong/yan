

#pragma once

#include <mongocxx/pool.hpp>

#include "base/event.h"
#include "base/ref_ops.h"
#include "com/macro.h"

namespace yan {
namespace mongo {

using MongoPool = mongocxx::pool;
using MongoPoolPtr = std::shared_ptr<MongoPool>;

class WriteEvent : public EventBase<int64_t> {
    using Base = EventBase<int64_t>;

public:
    WriteEvent(const MongoPoolPtr& pool) : pool_(pool) { assert(pool_); }
    ~WriteEvent() = default;

    template <typename RefObj>
    void prehandle(const RefObj& ro) {
        MakeWriteBson ops;
        doc_ = ops.make(ro);
    }

    void handle() {
        auto mongo_client = m_connpool->acquire();
        m_schema = mongo_client->uri().database();

        Base::set_exception(boost::make_exceptional(std::runtime_error("not implement")));
    }

private:
    const MongoPoolPtr& pool_;
    document doc_;
}

}  // namespace mongo
}  // namespace yan
