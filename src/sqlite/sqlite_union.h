

#pragma once

#include "base/event_ops.h"
#include "sqlite_event.h"

namespace yan {
namespace sqlite {
class SqliteUnion {
public:
    SqliteUnion(const EventOpsPtr &operators, const SqliteConnPoolPtr &pool)
        : operators_(operators), pool_(pool) {
        assert(operators_ && pool_);
    }
    ~SqliteUnion() {
        for (auto cond : condlist_)
            if (cond) delete cond;
    }

    inline void hold(std::shared_ptr<SqliteUnion> &ptr) { ptr_ = ptr; }

    template <typename Ret, typename... Args>
    inline SqliteUnion &union_read(Args &&...args);
    template <typename Ret, typename... Args>
    inline SqliteUnion &union_all(Args &&...args);

    template <typename Ret>
    inline future<Ret> commit();

private:
    template <typename Ret, typename... Args>
    inline SqliteUnion &read(Args &&...args);

private:
    const EventOpsPtr &operators_;
    const SqliteConnPoolPtr &pool_;

private:
    std::list<ReadCond *> condlist_;

private:
    std::shared_ptr<SqliteUnion> ptr_ = nullptr;
};

using SqliteUnionPtr = std::shared_ptr<SqliteUnion>;

/*******************************implementation*********************************/
template <typename Ret, typename... Args>
SqliteUnion &SqliteUnion::union_read(Args &&...args) {
    return read<Ret>(std::forward<Args>(args)...);
}
template <typename Ret, typename... Args>
SqliteUnion &SqliteUnion::union_all(Args &&...args) {
    return read<Ret>(std::forward<Args>(args)...);
}

template <typename Ret>
inline future<Ret> SqliteUnion::commit() {
    assert(operators_);
    return operators_->submit([this]() -> Ret {
        Ret ret;

        SqliteContextPtr ctx = nullptr;
        try {
            ctx = pool_->get();

            ctx->template read<Ret>(ret, condlist_);

            pool_->release(ctx);
        } catch (...) {
            if (ctx) pool_->release(ctx);
            boost::rethrow_exception(boost::current_exception());
        }

        return ret;
    });
}

template <typename Ret, typename... Args>
inline SqliteUnion &SqliteUnion::read(Args &&...args) {
    auto cond = new ReadCondImpl<Ret>();
    if (!cond) throw std::runtime_error("out of memory");

    cond->append(std::forward<Args>(args)...);

    condlist_.emplace_back(cond);
    return *this;
}
}  // namespace sqlite
}  // namespace yan
