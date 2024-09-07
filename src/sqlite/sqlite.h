

#pragma once

#include "base/type.h"
#include "sqlite_join.h"
#include "sqlite_union.h"
#include "sqlite_copy.h"

namespace yan {
namespace sqlite {

class Sqlite {
public:
    Sqlite() = default;
    ~Sqlite() = default;

    int init(const std::string &url) {
        operators_ = std::make_shared<EventOps>("sqlite3", 1);
        conn_pool_ = std::make_shared<SqliteConnPool>(url);

        return 0;
    }

    template <typename... Args>
    inline future<int64_t> create_table() {
        return create_event<int64_t, CreateTable<Args...>>();
    }

    template <typename... Args>
    inline future<int64_t> write(Args &&...args) {
        return create_event<int64_t, WriteEvent>(std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline future<int64_t> update(Args &&...args) {
        return create_event<int64_t, UpdateEvent<Args...>>(std::forward<Args>(args)...);
    }

    template <typename Ret, typename... Args>
    inline future<Ret> read(Args &&...args) {
        return create_event<Ret, ReadEvent<Ret>>(std::forward<Args>(args)...);
    }
    template <typename Ret, typename... Args>
    inline future<Ret> read(Ret &cond, Args &&...args) {
        return create_event<Ret, ReadEvent<Ret>>(cond, std::forward<Args>(args)...);
    }

    /**
     * @brief union read for SQL statements
     */
    template <typename Ret, typename... Args>
    inline SqliteUnion &union_read(Args &&...args) {
        auto u = std::make_shared<SqliteUnion>(operators_, conn_pool_);
        u->union_read<Ret>(std::forward<Args>(args)...);
        u->hold(u);
        return *u;
    }

    /**
     * @brief join read for SQL statement
     */
    template <typename CondObj>
    inline SqliteJoin &join_read() {
        auto j = std::make_shared<SqliteJoin>(operators_, conn_pool_);
        j->join<CondObj>();
        j->hold(j);
        return *j;
    }

    /**
     * @brief copy the entities from A to B with/without condition
     */
    template <typename From>
    inline SqliteCopy &copy() {
        auto c = std::make_shared<SqliteCopy>(operators_, conn_pool_);
        c->join<From>();
        c->hold(c);
        return *c;
    }

    template <typename Args>
    inline future<int64_t> del(Args &&args) {
        return create_event<int64_t, DeleteEvent>(std::forward<Args>(args));
    }

    template <typename Args>
    inline future<int64_t> count(Args &&args) {
        return create_event<int64_t, ReadEvent<int64_t>>(std::forward<Args>(args));
    }
    template <typename Args>
    inline future<int64_t> count() {
        auto event = std::make_shared<ReadEvent<int64_t>>(conn_pool_);
        try {
            event->template prehandle<Args>();
            assert(operators_);
            operators_->insert(event);
        } catch (const std::exception &e) {
            return make_yan_exception_future<int64_t>(std::runtime_error(e.what()));
        }

        return event->get_future();
    }

private:
    template <typename Ret, typename SqliteEvent, typename... Args>
    inline future<Ret> create_event(Args &&...args) {
        auto event = std::make_shared<SqliteEvent>(conn_pool_);
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
    SqliteConnPoolPtr conn_pool_ = nullptr;
};

}  // namespace sqlite
}  // namespace yan
