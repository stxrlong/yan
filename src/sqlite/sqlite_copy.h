
#pragma once

#include "sqlite_join.h"

namespace yan {
namespace sqlite {

class SqliteCopy : public SqliteJoinBase<SqliteCopy> {
    using Base = SqliteJoinBase<SqliteCopy>;

public:
    SqliteCopy(const EventOpsPtr &operators, const SqliteConnPoolPtr &pool)
        : Base(operators, pool) {}
    ~SqliteCopy() {
        // if (to_) delete to_;
        if (column_) delete column_;
    };

    inline void hold(std::shared_ptr<SqliteCopy> &ptr) { ptr_ = ptr; }

    // template <typename To, typename Column, typename... Args>
    // inline future<int64_t> to(Args &&...args);
    template <typename To, typename... Args>
    inline future<int64_t> to(Args &&...args);

private:
    template <typename CondObj, typename... Args>
    inline future<int64_t> commit(CondObj &&condobj, Args &&...args);

    inline future<int64_t> commit();

private:
    // JoinReadCond *to_ = nullptr;
    ReadCond *column_ = nullptr;
    std::shared_ptr<SqliteCopy> ptr_ = nullptr;
};

/*******************************implementation*********************************/
// template <typename To, typename Column, typename... Args>
// inline future<int64_t> SqliteCopy::to(Args &&...args) {
//     to_ = new JoinReadCond();
//     if (!to_) return make_yan_exception_future<int64_t>(std::runtime_error("out of memory"));
//     to_->append<To>();

//     column_ = new ReadCondImpl<Column>;
//     if (!column_) return make_yan_exception_future<int64_t>(std::runtime_error("out of memory"));

//     return commit(std::forward<Args>(args)...);
// }

template <typename To, typename... Args>
inline future<int64_t> SqliteCopy::to(Args &&...args) {
    column_ = new ReadCondImpl<To>;
    if (!column_) make_yan_exception_future<int64_t>(std::runtime_error("out of memory"));

    return commit(std::forward<Args>(args)...);
}

template <typename CondObj, typename... Args>
inline future<int64_t> SqliteCopy::commit(CondObj &&condobj, Args &&...args) {
    try {
        this->template set_cond(condobj);
    } catch (const std::exception &e) {
        return make_yan_exception_future<int64_t>(std::runtime_error(e.what()));
    }

    return commit(std::forward<Args>(args)...);
}

inline future<int64_t> SqliteCopy::commit() {
    assert(operators_);
    if (joincondlist_.size() < 1)
        return make_yan_exception_future<int64_t>(
            std::runtime_error("sqlite copy operation requires the from and to tables"));

    return operators_->submit([this]() -> int64_t {
        int64_t ret;

        SqliteContextPtr ctx = nullptr;
        try {
            ctx = pool_->get();

            ctx->copy(ret, joincondlist_, /* to_, */ column_, condlist_);

            pool_->release(ctx);
        } catch (...) {
            if (ctx) pool_->release(ctx);
            boost::rethrow_exception(boost::current_exception());
        }

        return ret;
    });
}

}  // namespace sqlite
}  // namespace yan
