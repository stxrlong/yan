

#pragma once

#include "mongo_readref.h"

namespace yan {
namespace mongo {

class FetchMongoResult {
    class WriteMemRef;
    class WriteListMem;
    class WriteStructMem;
    using MongoWriteRefOps = WriteRefOps<const view, WriteMemRef, WriteListMem, WriteStructMem>;

    MongoWriteRefOps ops_;

public:
    FetchMongoResult() = default;
    ~FetchMongoResult() = default;

    template <typename RefObj>
    inline int fetch(RefObj &ro, const view &view) {
        int ret = ops_.get(ro, view);
        if (ret < 0) throw std::runtime_error("mongo fetch result failed: " + std::to_string(ret));

        return ret;
    }

private:
    /**************************Write Member Ref***********************/
    class WriteMemRef {
    public:
        WriteMemRef(const view &view, const std::string &name) : view_(view), name_(name) {}
        ~WriteMemRef() = default;

        template <typename T>
        inline int get(const std::function<int(T &&)> &cb) {
            return get_value(cb);
        }

    private:
        template <typename T>
        inline int get_value(const std::function<int(T &&)> &cb) {
            throw std::runtime_error("mongo does not implement bind value func");
            return 0;
        }

        template <>
        inline int get_value(const std::function<int(std::string &&)> &cb) {
            auto v = view_[name_];
            if (v) {
                std::string t = v.get_utf8().value.to_string();
                cb(std::move(t));
            }
            return 0;
        }

#define FETCH_VALUE_TYPE(fetchfunc, T)                         \
    template <>                                                \
    inline int get_value(const std::function<int(T &&)> &cb) { \
        auto v = view_[name_];                                 \
        if (v) {                                               \
            T t = v.fetchfunc();                               \
            cb(std::move(t));                                  \
        }                                                      \
        return 0;                                              \
    }

        FETCH_VALUE_TYPE(get_int32, int32_t)
        FETCH_VALUE_TYPE(get_double, double)

#undef FETCH_VALUE_TYPE

    private:
        const std::string &name_;
        const view &view_;
    };

    /**************************Write List Member***********************/
    class WriteListMem {
    public:
        WriteListMem(const view &view, const std::string &name) {
            auto ele = view[name];
            if (ele && ele.type() == bsoncxx::type::k_array) {
                array_ = std::move(ele.get_array());
            }
        }
        ~WriteListMem() = default;

        inline int get(const RefOpsCallback<const view> &cb) {
            int ret = 0;
            for (int i = 0;; ++i) {
                auto v = array_.value[i];
                if (!v) break;

                if ((ret = cb(v.get_document().view())) < 0) break;
            }

            return ret;
        }

        template <typename T>
        inline int get(const WriteRefCallback<T> &cb) {
            return get_value(cb);
        }

    private:
        template <typename T>
        inline int get_value(const WriteRefCallback<T> &cb) {
            throw std::runtime_error("mongo does not implement get value func");
            return 0;
        }
        template <>
        inline int get_value(const WriteRefCallback<std::string> &cb) {
            for (int i = 0;; ++i) {
                auto v = array_.value[i];
                if (!v) break;

                auto t = v.get_utf8().value.to_string();
                cb(std::move(t));
            }

            return 0;
        }

#define FETCH_VALUE_TYPE(fetchfunc, T)                         \
    template <>                                                \
    inline int get_value(const std::function<int(T &&)> &cb) { \
        for (int i = 0;; ++i) {                                \
            auto v = array_.value[i];                          \
            if (!v) break;                                     \
                                                               \
            auto t = v.fetchfunc();                            \
            cb(std::move(t));                                  \
        }                                                      \
        return 0;                                              \
    }

        FETCH_VALUE_TYPE(get_int32, int32_t)
        FETCH_VALUE_TYPE(get_double, double)

#undef FETCH_VALUE_TYPE

    private:
        bsoncxx::types::b_array array_;
    };

    /**************************Write Struct Member***********************/
    class WriteStructMem {
    public:
        WriteStructMem(const view &view, const std::string &name) : view_(view), name_(name) {}

        int get(const RefOpsCallback<const view> &cb) {
            auto e = view_[name_];
            if (e) return cb(e.get_document().view());

            return 0;
        }

    private:
        const std::string &name_;
        const view &view_;
    };
};

}  // namespace mongo
}  // namespace yan
