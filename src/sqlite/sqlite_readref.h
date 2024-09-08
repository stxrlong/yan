
#pragma once

#include "base/ref_ops.h"
#include "com/logger.h"

extern "C" {
#include "sqlite3.h"
}

namespace yan {
namespace sqlite {

/************************************create table sql****************************************/
class MakeTableSql {
    struct SqlContext {
        std::string fields_;

        bool append_first_ = true;
    } sqlctx_;

    class ReadMemRef;
    using TableSqlOps = ComReadRef<SqlContext, ReadMemRef>::ComReadRefOps;
    TableSqlOps tableops_;

public:
    MakeTableSql() {}

    template <typename Table>
    inline std::string make() {
        auto &name = get_obj_name<Table>();

        int ret = tableops_.append_field<Table>(sqlctx_);
        if (ret < 0)
            throw std::runtime_error("create sqlite sql for table [" + name +
                                     "] failed: " + std::to_string(ret));
        if (sqlctx_.append_first_)
            throw std::runtime_error("sqlite has no field set when creating table sql [" + name +
                                     "]");

        return create(name);
    }

private:
    std::string create(const std::string &table);

private:
    class ReadMemRef {
    public:
        ReadMemRef(SqlContext &sqlctx) : sqlctx_(sqlctx) {}

        int append(const std::string &name, const YanType type, const bool canbenull,
                   const int32_t length);

    private:
        SqlContext &sqlctx_;
    };
};

/*************************************MakeInsertSql****************************************/
class MakeInsertSql {
    struct SqlContext {
        std::string fields_;
        std::string values_;

        bool append_first_ = true;
        bool onlyone_ = true;
        void reset() {
            values_ = "";
            append_first_ = true;
            onlyone_ = false;
        }
    } sqlctx_;

    class ReadMemRef;
    using InsertSqlOps = ComReadRef<SqlContext, ReadMemRef>::ComReadRefOps;
    InsertSqlOps insertops_;

public:
    MakeInsertSql() {}

    template <typename InsertObj,
              typename std::enable_if<!IsSequenceContainer<InsertObj>::value, int>::type = 0>
    inline std::string make(const InsertObj &insertobj) {
        auto &name = get_obj_name<InsertObj>();

        int ret = insertops_.append(sqlctx_, insertobj);
        if (ret < 0)
            throw std::runtime_error("sqlite append insert sql failed: [" + name +
                                     "] failed: " + std::to_string(ret));
        if (sqlctx_.append_first_)
            throw std::runtime_error("sqlite has no field set when creating insert sql");

        return create(name);
    }

    template <typename InsertObj,
              typename std::enable_if<IsSequenceContainer<InsertObj>::value, int>::type = 0>
    inline std::string make(const InsertObj &insertobj) {
        int count = 0;
        std::string insertsql;
        for (auto &obj : insertobj) {
            if (++count == 1) {
                insertsql = make(obj);
                continue;
            }

            sqlctx_.reset();
            int ret = insertops_.append(sqlctx_, obj);
            if (ret < 0)
                throw std::runtime_error("sqlite append insert list sql failed: " +
                                         std::to_string(ret));
            insertsql.append(", (").append(sqlctx_.values_).append(")");
        }

        return insertsql;
    }

private:
    std::string create(const std::string &name);

private:
    class ReadMemRef {
    public:
        ReadMemRef(SqlContext &sqlctx, const std::string &name) : sqlctx_(sqlctx), name_(name) {}

        template <typename T>
        inline int append(const CondType condtype, const YanOpsType opstype,
                          const std::function<const T &()> &cb) {
            if (condtype != CondType::AND) return -1;

            if (!sqlctx_.append_first_) {
                if (sqlctx_.onlyone_) sqlctx_.fields_.append(", ");
                sqlctx_.values_.append(", ");
            } else {
                sqlctx_.append_first_ = false;
            }

            append_value(cb);
            return 0;
        }

    private:
        template <typename T>
        void append_value(const std::function<const T &()> &cb);

    private:
        const std::string &name_;
        SqlContext &sqlctx_;
    };
};

/*************************************MakeQuerySql****************************************/
class MakeQuerySql {
protected:
    struct SqlContext {
        std::string sql_;
        bool append_ret_first = true;
        bool append_cond_first = true;
    } sqlctx_;

    class ReadMemRef;
    using QuerySqlOps = ComReadRef<SqlContext, ReadMemRef>::ComReadRefOps;
    QuerySqlOps queryops_;

public:
    MakeQuerySql() {}

    /**
     * @brief query from one table
     */
    std::string make(const ReadCond *cond);

    /**
     * @brief union query from at least two tables
     */
    std::string make(const std::list<ReadCond *> &conds);

    /**
     * @brief join query from at lease two tables
     */
    std::string make(const ReadCond *retcond, const std::list<JoinReadCond *> &equals,
                     const std::list<ReadCond *> &conds);

    /**
     * @brief copy sql
     */
    std::string make(const std::list<JoinReadCond *> &froms, /* const JoinReadCond *to, */
                     const ReadCond *column, const std::list<ReadCond *> &conds);

    virtual std::string make(const UpdateCond *cond) { return ""; }

private:
    std::string left_join(const std::list<JoinReadCond *> &equals,
                          const std::list<ReadCond *> &conds);

protected:
    class ReadMemRef {
    public:
        ReadMemRef(SqlContext &sqlctx, const std::string &name) : sqlctx_(sqlctx), name_(name) {}

        template <typename T>
        inline int append(const CondType condtype, const YanOpsType opstype,
                          const std::function<const T &()> &cb) {
            return append_cond(condtype, opstype);
        }

    private:
        int append_cond(const CondType condtype, const YanOpsType opstype);

    private:
        const std::string &name_;
        SqlContext &sqlctx_;
    };
};

/*************************************MakeUpdateSql***************************************/
class MakeUpdateSql : protected MakeQuerySql {
    struct FieldContext {
        std::string fields_;

        bool append_first_ = true;
    } fieldctx_;

    class ReadFieldMemRef;
    using UpdateSqlOps = ComReadRef<FieldContext, ReadFieldMemRef>::ComReadRefOps;
    UpdateSqlOps updateops_;

public:
    MakeUpdateSql() {}

    std::string make(const UpdateCond *cond);

private:
    class ReadFieldMemRef {
    public:
        ReadFieldMemRef(FieldContext &fldctx, const std::string &name)
            : fldctx_(fldctx), name_(name) {}

        template <typename T>
        inline int append(const CondType condtype, const YanOpsType opstype,
                          const std::function<const T &()> &cb) {
            if (!fldctx_.append_first_) {
                fldctx_.fields_.append(", ");
            } else {
                fldctx_.append_first_ = false;
            }

            fldctx_.fields_.append(name_).append(" = ?");
            return 0;
        }

    private:
        const std::string &name_;
        FieldContext &fldctx_;
    };
};

/*************************************MakeDeleteSql***************************************/
class MakeDeleteSql : protected MakeQuerySql {
public:
    MakeDeleteSql() {}

    std::string make(const ReadCond *cond);
};

/*************************************SetParameter****************************************/
class BindSqlParam {
    struct BindContext {
        sqlite3_stmt *stmt_ = nullptr;
        int index_ = 0;
    } bindctx_;

    class ReadMemRef;
    using BindSqlOps = ComReadRef<BindContext, ReadMemRef>::ComReadRefOps;
    BindSqlOps bindops_;

public:
    BindSqlParam(sqlite3_stmt *stmt) { bindctx_.stmt_ = stmt; }

    inline void bind(const ReadCond *cond) {
        if (!cond->has_cond()) return;

        int ret = bindops_.append_cond(bindctx_, *cond);
        if (ret < 0)
            throw std::runtime_error("sqlite bind condition for query sql failed: " +
                                     std::to_string(ret));

        if (bindctx_.index_ == 0) throw std::runtime_error("no parameter was binded");
    }

    inline void bind(const std::list<ReadCond *> &conds) {
        for (auto cond : conds) {
            assert(cond);
            bind(cond);
        }
    }

    inline void bind(const ReadCond *, const std::list<JoinReadCond *> &,
                     const std::list<ReadCond *> &conds) {
        for (auto cond : conds) {
            assert(cond);
            bind(cond);
        }
    }

    inline void bind(const std::list<JoinReadCond *> &,
                     /* const JoinReadCond *, */ const ReadCond *,
                     const std::list<ReadCond *> &conds) {
        for (auto cond : conds) {
            assert(cond);
            bind(cond);
        }
    }

    inline void bind(const UpdateCond *cond) {
        int ret = bindops_.append_value(bindctx_, *cond);
        if (ret < 0)
            throw std::runtime_error("sqlite bind value for update sql failed: " +
                                     std::to_string(ret));

        return bind(static_cast<const ReadCond *>(cond));
    }

private:
    class ReadMemRef {
    public:
        ReadMemRef(BindContext &bindctx, const std::string &name)
            : bindctx_(bindctx), name_(name) {}

        template <typename T>
        inline int append(const CondType condtype, const YanOpsType opstype,
                          const std::function<const T &()> &cb) {
            ++bindctx_.index_;
            return append_value(cb);
        }

    private:
        template <typename T>
        inline int append_value(const std::function<const T &()> &cb) {
            throw std::runtime_error("sqlite does not implement bind value func");
            return 0;
        }

        template <>
        inline int append_value(const std::function<const std::string &()> &cb) {
            int ret =
                sqlite3_bind_text(bindctx_.stmt_, bindctx_.index_, cb().c_str(), -1, SQLITE_STATIC);
            if (ret != SQLITE_OK) {
                std::string error = "sqlite bind [" + name_ + ":" +
                                    std::to_string(bindctx_.index_) +
                                    "] value failed: " + sqlite3_errstr(ret);
                throw std::runtime_error(error);
            }

            return 0;
        }

#define BIND_VALUE_TYPE(bindfunc, T)                                                              \
    template <>                                                                                   \
    inline int append_value(const std::function<const T &()> &cb) {                               \
        int ret = bindfunc(bindctx_.stmt_, bindctx_.index_, cb());                                \
        if (ret != SQLITE_OK) {                                                                   \
            std::string error = "sqlite bind [" + name_ + ":" + std::to_string(bindctx_.index_) + \
                                "] value failed: " + sqlite3_errstr(ret);                         \
            throw std::runtime_error(error);                                                      \
        }                                                                                         \
        return 0;                                                                                 \
    }

        BIND_VALUE_TYPE(sqlite3_bind_int, int32_t)
        BIND_VALUE_TYPE(sqlite3_bind_int64, int64_t)

#undef BIND_VALUE_TYPE

    private:
        const std::string &name_;
        BindContext &bindctx_;
    };
};

}  // namespace sqlite
}  // namespace yan
