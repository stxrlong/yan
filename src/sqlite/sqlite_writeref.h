#pragma once

#include "base/ref_ops.h"
#include "com/logger.h"

extern "C" {
#include "sqlite3.h"
}

namespace yan {
namespace sqlite {
/*************************************SqliteWriteRef****************************************/
class SqliteWriteRef {
protected:
    class WriteListRef {
    public:
        WriteListRef() {}
    };

    class WriteStructRef {
    public:
        WriteStructRef() {}
    };

    template <typename Context, typename MemRef>
    using SqliteWriteRefOps = WriteRefOps<Context, MemRef, WriteListRef, WriteStructRef>;
};

/*************************************FetchResult****************************************/
class FetchSqlResult : public SqliteWriteRef {
    struct FetchContext {
        sqlite3_stmt *stmt_ = nullptr;
        int col_ = -1;
    } fetchctx_;

    class WriteMemRef;
    using FetchSqlOps = SqliteWriteRefOps<FetchContext, WriteMemRef>;
    FetchSqlOps fetchops_;

public:
    FetchSqlResult(sqlite3_stmt *stmt) { fetchctx_.stmt_ = stmt; }

    template <typename CondObj>
    inline void fetch(CondObj &condobj) {
        int ret = fetchops_.get(fetchctx_, condobj);
        if (ret < 0) throw std::runtime_error("sqlite fetch result failed: " + std::to_string(ret));
    }

	template<>
    inline void fetch(int64_t &count) { count = (int64_t)sqlite3_column_int64(fetchctx_.stmt_, 0); }

private:
    class WriteMemRef {
    public:
        WriteMemRef(FetchContext &fetchctx, const std::string &name) : fetchctx_(fetchctx) {}

        template <typename T>
        inline int get(const std::function<int(T &&)> &cb) {
            ++fetchctx_.col_;
            return get_value(cb);
        }

    private:
        template <typename T>
        inline int get_value(const std::function<int(T &&)> &cb) {
            throw std::runtime_error("sqlite does not implement bind value func");
            return 0;
        }

        template <>
        inline int get_value(const std::function<int(std::string &&)> &cb) {
            const char *t = (char *)sqlite3_column_text(fetchctx_.stmt_, fetchctx_.col_);
            if (t) {
                assert(cb);
                std::string v(t);
                cb(std::move(t));
            }
            return 0;
        }

#define FETCH_VALUE_TYPE(fetchfunc, T)                         \
    template <>                                                \
    inline int get_value(const std::function<int(T &&)> &cb) { \
        T t = (T)fetchfunc(fetchctx_.stmt_, fetchctx_.col_);   \
        assert(cb);                                            \
        cb(std::move(t));                                      \
        return 0;                                              \
    }

        FETCH_VALUE_TYPE(sqlite3_column_int, int32_t)
        FETCH_VALUE_TYPE(sqlite3_column_int64, int64_t)

#undef FETCH_VALUE_TYPE

    private:
        FetchContext &fetchctx_;
    };
};

}  // namespace sqlite
}  // namespace yan