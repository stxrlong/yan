

#pragma once

#include "base/event.h"
#include "com/macro.h"
#include "sqlite_context.h"

namespace yan {
namespace sqlite {

/*******************************write event*********************************/
class WriteEvent : public EventBase<int64_t> {
    using Base = EventBase<int64_t>;

public:
    WriteEvent(const SqliteConnPoolPtr& pool) : pool_(pool) { assert(pool_); }
    ~WriteEvent() = default;

    template <typename RefObj,
              typename std::enable_if<!IsSequenceContainer<RefObj>::value, int>::type = 0>
    void prehandle(RefObj& obj) {
        MakeInsertSql sqlops;
        auto sql = sqlops.make(obj);
        sql.append(";");
        sqls_.emplace_back(std::move(sql));
        entity_ += 1;
    }

    template <typename RefObj,
              typename std::enable_if<IsSequenceContainer<RefObj>::value, int>::type = 0>
    void prehandle(RefObj& obj) {
        /**
         * @brief we cannot know how many entities at one insertation, therefore,
         * we only use statement in this operation
         */
        MakeInsertSql sqlops;
        auto sql = sqlops.make(obj);
        sql.append(";");
        sqls_.emplace_back(std::move(sql));
        entity_ += obj.size();
    }

    template <typename RefObj, typename... Args>
    auto prehandle(RefObj&& obj, Args&&... args) ->
        typename std::enable_if<(sizeof...(args) > 0), void>::type {
        prehandle(obj);
        prehandle(std::forward<Args>(args)...);
    }

    void handle() {
        SqliteContextPtr ctx = nullptr;
        try {
            ctx = pool_->get();

            for (auto& sql : sqls_) ctx->write(sql);

            pool_->release(ctx);
        } catch (...) {
            if (ctx) pool_->release(ctx);

            Base::set_exception(boost::current_exception());
            return;
        }

        Base::set_value(entity_);
    }

private:
    const SqliteConnPoolPtr& pool_;
    std::list<std::string> sqls_;
    int entity_ = 0;
};

/*******************************update event*********************************/
template <typename RefObj>
class UpdateEvent : public EventBase<int64_t> {
    using Base = EventBase<int64_t>;

public:
    UpdateEvent(const SqliteConnPoolPtr& pool) : pool_(pool) { assert(pool_); }
    ~UpdateEvent() = default;

    void prehandle(RefObj& obj) {
        /**
         * @brief we have no choice to copy the parameters for this asynchronous operator,
         * because the sqlite's preparedstatement does not support to be reused when it
         * is under sqlite_step, so as the mysql. therefore, these asynchronous design will
         * not reduce the connections but can reduce the use of threads.
         */
        cond_.copy_refobj(obj);
    }

    void handle() {
        SqliteContextPtr ctx = nullptr;
        try {
            ctx = pool_->get();

            ctx->update(&cond_);

            pool_->release(ctx);
        } catch (...) {
            if (ctx) pool_->release(ctx);

            Base::set_exception(boost::current_exception());
            return;
        }

        Base::set_value(1);
    }

private:
    const SqliteConnPoolPtr& pool_;

    UpdateCondImpl<RefObj> cond_;
};

/*******************************read event*********************************/
template <typename Ret>
class ReadEvent : public EventBase<Ret> {
    using Base = EventBase<Ret>;

public:
    ReadEvent(const SqliteConnPoolPtr& pool) : pool_(pool) { assert(pool_); }
    ~ReadEvent() = default;

    template <typename... Args>
    inline void prehandle(Args&&... args) {
        return cond_.append(std::forward<Args>(args)...);
    }

    template <typename Args>
    inline void prehandle() {
        return cond_.template append<Args>();
    }

    void handle() {
        Ret ret;

        SqliteContextPtr ctx = nullptr;
        try {
            ctx = pool_->get();

            ctx->template read<Ret>(ret, &cond_);

            pool_->release(ctx);
        } catch (...) {
            if (ctx) pool_->release(ctx);

            Base::set_exception(boost::current_exception());
            return;
        }

        Base::set_value(std::move(ret));
    }

private:
    const SqliteConnPoolPtr& pool_;

    ReadCondImpl<Ret> cond_;
};

/*******************************delete event*********************************/
class DeleteEvent : public EventBase<int64_t> {
    using Base = EventBase<int64_t>;

public:
    DeleteEvent(const SqliteConnPoolPtr& pool) : pool_(pool) { assert(pool_); }
    ~DeleteEvent() = default;

    template <typename... Args>
    inline void prehandle(Args&&... args) {
        return cond_.append(std::forward<Args>(args)...);
    }

    void handle() {
        int64_t ret = 0;

        SqliteContextPtr ctx = nullptr;
        try {
            ctx = pool_->get();

            ctx->del(ret, &cond_);

            pool_->release(ctx);
        } catch (...) {
            if (ctx) pool_->release(ctx);

            Base::set_exception(boost::current_exception());
            return;
        }

        Base::set_value(ret);
    }

private:
    const SqliteConnPoolPtr& pool_;

    ReadCondImpl<int64_t> cond_;
};

/*******************************create table*********************************/
template <typename... RefObjs>
class CreateTable : public EventBase<int64_t> {
    using Base = EventBase<int64_t>;

public:
    CreateTable(const SqliteConnPoolPtr& pool) : pool_(pool) { assert(pool_); }
    ~CreateTable() = default;

    void prehandle() {
        make_create_sql<RefObjs...>();

        entity_ = sizeof...(RefObjs);
    }

    void handle() {
        if (sqls_.size() == 0)
            throw std::runtime_error("please specify at least one table when creating tables");

        SqliteContextPtr ctx = nullptr;
        try {
            ctx = pool_->get();

            for (auto& sql : sqls_) ctx->write(sql);

            pool_->release(ctx);
        } catch (...) {
            if (ctx) pool_->release(ctx);

            Base::set_exception(boost::current_exception());
            return;
        }

        Base::set_value(entity_);
    }

private:
    template <typename Table, typename... ROs>
    auto make_create_sql() -> typename std::enable_if<(sizeof...(ROs) > 0), void>::type {
        make_create_sql<Table>();
        make_create_sql<ROs...>();
    }

    template <typename Table>
    void make_create_sql() {
        Table table;
        MakeTableSql tableops;
        auto sql = tableops.make<Table>();
        sql.append(";");

        sqls_.emplace_back(std::move(sql));
    }

private:
    const SqliteConnPoolPtr& pool_;
    std::list<std::string> sqls_;
    int entity_ = 0;
};

}  // namespace sqlite
}  // namespace yan
