#pragma once

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>

#include "mongo_writeref.h"

namespace yan {
namespace mongo {

class MongoContext {
    using MongoPool = mongocxx::pool;
    using MongoPoolPtr = std::shared_ptr<MongoPool>;

public:
    MongoContext() = default;
    ~MongoContext() = default;

    int init(const std::string &uri) {
        mongocxx::instance instance{};
        try {
            mongocxx::uri mongouri(uri);
            pool_ = std::make_shared<MongoPool>(mongouri);
        } catch (const std::exception &e) {
            logger_debug("init mongodb with uri[%s] failed", uri.c_str());
            return -1;
        }

        return 0;
    }

    inline void write(const std::string &table, std::vector<document> &docs) {
        assert(pool_);
        auto client = pool_->acquire();
        auto coll = (*client)[schema_][table];
        coll.insert_many(docs);
    }

    template <typename RetObj>
    inline int read(RetObj &ro, const std::string &table, document &doc) {
        assert(pool_);
        auto client = pool_->acquire();
        auto coll = (*client)[schema_][table];
        auto cursor = coll.find(doc.view());

        return fetch_value(ro, cursor);
    }

    inline int del(int64_t &ret, const std::string &table, document &doc) {
        assert(pool_);
        auto client = pool_->acquire();
        auto coll = (*client)[schema_][table];
        auto cursor = coll.delete_many(doc.view());

        ret = cursor->deleted_count();
        return 0;
    }

private:
    template <typename RetObj>
    auto fetch_value(RetObj &ro, mongocxx::cursor &cursor) ->
        typename std::enable_if<!IsSequenceContainer<RetObj>::value, int>::type {
        for (const auto &view : cursor) {
            FetchMongoResult ops;
            ops.fetch(ro, view);
            return 1;
        }

        return 0;
    }

    template <typename RetObj>
    auto fetch_value(RetObj &ro, mongocxx::cursor &cursor) ->
        typename std::enable_if<IsSequenceContainer<RetObj>::value, int>::type {
        using Type =
            typename std::decay<decltype(std::declval<typename RetObj::value_type>())>::type;

        int count = 0;
        for (const auto &view : cursor) {
            Type obj;

            FetchMongoResult ops;
            ops.fetch(obj, view);

            ro.emplace_back(std::move(obj));
            ++count;
        }

        return count;
    }

private:
    MongoPoolPtr pool_;
    std::string schema_;
};

using MongoContextPtr = std::shared_ptr<MongoContext>;

}  // namespace mongo
}  // namespace yan
