

#pragma once

#include "base/event.h"
#include "base/ref_ops.h"
#include "com/macro.h"
#include "mongo_context.h"

namespace yan {
namespace mongo {

/**************************Write Event***********************/
class WriteEvent : public EventBase<int64_t> {
    using Base = EventBase<int64_t>;

public:
    WriteEvent(const MongoContextPtr& context) : context_(context) { assert(context); }
    ~WriteEvent() = default;

    template <typename RefObj,
              typename std::enable_if<!IsSequenceContainer<RefObj>::value, int>::type = 0>
    void prehandle(const RefObj& ro) {
        MakeWriteBson ops;
        auto doc = ops.make(ro);
        docs_.emplace_back(std::move(doc));

        ++entities_;
        coll_ = get_obj_name<RefObj>();
    }

    template <typename RefObj,
              typename std::enable_if<IsSequenceContainer<RefObj>::value, int>::type = 0>
    void prehandle(const RefObj& ro) {
        using Type =
            typename std::decay<decltype(std::declval<typename RefObj::value_type>())>::type;
        coll_ = get_obj_name<Type>();

        for (auto& e : ro) {
            MakeWriteBson ops;
            auto doc = ops.make(e);
            docs_.emplace_back(std::move(doc));
        }

        entities_ = ro.size();
    }

    void handle() {
        assert(context_);

        try {
            context_->write(coll_, docs_);
        } catch (...) {
            Base::set_exception(boost::current_exception());
            return;
        }

        Base::set_value(entities_);
    }

private:
    const MongoContextPtr& context_;
    int64_t entities_ = 0;

    std::string coll_;
    std::vector<document> docs_;
};

/**************************Read Event***********************/
template <typename Ret>
class ReadEvent : public EventBase<Ret> {
    using Base = EventBase<Ret>;

public:
    ReadEvent(const MongoContextPtr& context) : context_(context) { assert(context); }
    ~ReadEvent() = default;

    template <typename RefObj>
    void prehandle(const RefObj& ro) {
        MakeCondBson ops;
        doc_ = ops.make(ro);
        coll_ = get_obj_name<RefObj>();
    }

    void handle() {
        assert(context_);

        Ret ret;
        try {
            context_->read(ret, coll_, doc_);
        } catch (...) {
            Base::set_exception(boost::current_exception());
            return;
        }

        Base::set_value(std::move(ret));
    }

private:
    const MongoContextPtr& context_;
    std::string coll_;
    document doc_;
};

/**************************Delete Event***********************/
class DeleteEvent : public EventBase<int64_t> {
    using Base = EventBase<int64_t>;

public:
    DeleteEvent(const MongoContextPtr& context) : context_(context) { assert(context); }
    ~DeleteEvent() = default;

    template <typename RefObj>
    void prehandle(const RefObj& ro) {
        MakeCondBson ops;
        doc_ = ops.make(ro);
        coll_ = get_obj_name<RefObj>();
    }

    void handle() {
        assert(context_);

        int64_t ret;
        try {
            context_->del(ret, coll_, doc_);
        } catch (...) {
            Base::set_exception(boost::current_exception());
            return;
        }

        Base::set_value(std::move(ret));
    }

private:
    const MongoContextPtr& context_;
    std::string coll_;
    document doc_;
};

}  // namespace mongo
}  // namespace yan
