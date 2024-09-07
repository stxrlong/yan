

#pragma once

#include "base/event_ops.h"
#include "sqlite_event.h"

namespace yan {
namespace sqlite {

template <typename Concrete>
class SqliteJoinBase {
public:
    SqliteJoinBase(const EventOpsPtr &operators, const SqliteConnPoolPtr &pool)
        : operators_(operators), pool_(pool) {
        assert(operators_ && pool_);
    }
    ~SqliteJoinBase() {
        for (auto cond : joincondlist_)
            if (cond) delete cond;
        for (auto cond : condlist_)
            if (cond) delete cond;
    };

    template <typename RefObj, typename... Args>
    inline Concrete &join(Args &&...args);
    template <typename RefObj, typename... Args>
    inline Concrete &left_join(Args &&...args);
    template <typename RefObj, typename... Args>
    inline Concrete &right_join(Args &&...args) {}

protected:
    template <typename CondObj>
    inline void set_cond(CondObj &&condobj);

protected:
    const EventOpsPtr &operators_;
    const SqliteConnPoolPtr &pool_;

protected:
    std::list<JoinReadCond *> joincondlist_;
    std::list<ReadCond *> condlist_;
};

class SqliteJoin : public SqliteJoinBase<SqliteJoin> {
    using Base = SqliteJoinBase<SqliteJoin>;

public:
    SqliteJoin(const EventOpsPtr &operators, const SqliteConnPoolPtr &pool)
        : Base(operators, pool) {}
    ~SqliteJoin() = default;

    inline void hold(std::shared_ptr<SqliteJoin> &ptr) { ptr_ = ptr; }

    template <typename Ret, typename CondObj, typename... Args>
    inline future<Ret> commit(CondObj &&condobj, Args &&...args);
    template <typename Ret>
    inline future<Ret> commit();

private:
    std::shared_ptr<SqliteJoin> ptr_ = nullptr;
};

using SqliteJoinPtr = std::shared_ptr<SqliteJoin>;

/*******************************implementation*********************************/
template <typename Concrete>
template <typename RefObj, typename... Args>
inline Concrete &SqliteJoinBase<Concrete>::join(Args &&...args) {
    auto cond = new JoinReadCond();
    if (!cond) throw std::runtime_error("out of memory");

    cond->append<RefObj>(std::forward<Args>(args)...);
    joincondlist_.emplace_back(cond);
    return *(static_cast<Concrete *>(this));
}

template <typename Concrete>
template <typename RefObj, typename... Args>
inline Concrete &SqliteJoinBase<Concrete>::left_join(Args &&...args) {
    return join<RefObj>(std::forward<Args>(args)...);
}

template <typename Concrete>
template <typename CondObj>
inline void SqliteJoinBase<Concrete>::set_cond(CondObj &&condobj) {
    auto cond = new ReadCondImpl<CondObj>();
    if (!cond) throw std::runtime_error("out of memory");

    cond->template append(std::forward<CondObj>(condobj));
    condlist_.emplace_back(cond);
}

template <typename Ret, typename CondObj, typename... Args>
inline future<Ret> SqliteJoin::commit(CondObj &&condobj, Args &&...args) {
    try {
        this->template set_cond(condobj);
    } catch (const std::exception &e) {
        return make_yan_exception_future<Ret>(std::runtime_error(e.what()));
    }
    return commit<Ret>(std::forward<Args>(args)...);
}

template <typename Ret>
inline future<Ret> SqliteJoin::commit() {
    assert(operators_);
    if (joincondlist_.size() < 2)
        return make_yan_exception_future<Ret>(
            std::runtime_error("sqlite join read requires at least two tables"));

    return operators_->submit([this]() -> Ret {
        Ret ret;

        SqliteContextPtr ctx = nullptr;
        try {
            ctx = pool_->get();

            ctx->template read<Ret>(ret, joincondlist_, condlist_);

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
