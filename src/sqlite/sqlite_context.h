#pragma once

#include <mutex>
#include <thread>
#include <unordered_map>

#include "base/ref_ops.h"
#include "com/conn_pool.h"
#include "com/logger.h"
#include "sqlite_readref.h"
#include "sqlite_writeref.h"

namespace yan {
namespace sqlite {

class SqliteContext {
public:
    SqliteContext(const std::string &path);
    ~SqliteContext();

    inline void write(const std::string &sql);

    inline void update(const UpdateCond *cond);

    inline void del(int64_t &ret, const ReadCond *cond);

    inline void copy(int64_t &ret,
                     const std::list<JoinReadCond *> &froms, /* const JoinReadCond *to, */
                     const ReadCond *column, const std::list<ReadCond *> &conds);

    template <typename RetObj>
    inline void read(RetObj &ro, const ReadCond *cond);

    template <typename RetObj>
    inline void read(RetObj &ro, const std::list<ReadCond *> &conds);

    template <typename RetObj>
    inline void read(RetObj &ro, const std::list<JoinReadCond *> &equals,
                     const std::list<ReadCond *> &conds);

private:
    template <typename MakeSql, typename... Args>
    inline void execute(const std::string &hash, const std::function<void(sqlite3_stmt *)> &exe,
                        Args &&...args);

    template <typename MakeSql, typename... Args>
    inline sqlite3_stmt *create_stmt(Args &&...args);

    template <typename RetObj,
              typename std::enable_if<!IsSequenceContainer<RetObj>::value, int>::type = 0>
    inline void exec(RetObj &ro, sqlite3_stmt *stmt);

    template <typename RetObj,
              typename std::enable_if<IsSequenceContainer<RetObj>::value, int>::type = 0>
    inline void exec(RetObj &ro, sqlite3_stmt *stmt);

    inline void exec_entity(int64_t &ret, sqlite3_stmt *stmt);

    template <typename RetObj>
    inline void fetch_value(RetObj &ro, sqlite3_stmt *stmt);

private:
    inline std::string errmsg(const std::string &selfdef, const int ret);
    inline void set_sqlite_pragma(sqlite3 *conn);

private:
    sqlite3 *conn_ = nullptr;

    std::unordered_map<std::string, sqlite3_stmt *> read_stmts_;
};

using SqliteContextPtr = std::shared_ptr<SqliteContext>;

using SqliteConnPool = ConnectionPool<SqliteContext>;
using SqliteConnPoolPtr = std::shared_ptr<SqliteConnPool>;

/*******************************implementation*********************************/
inline SqliteContext::SqliteContext(const std::string &path) {
    if (sqlite3_open_v2(path.c_str(), &conn_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                        nullptr) != SQLITE_OK) {
        conn_ = nullptr;
        std::string error = std::string("create sqlite connection failed with path: ") + path;
        throw std::runtime_error(error);
    }

    logger_info("open sqlite db ok, path: %s", path.c_str());
    set_sqlite_pragma(conn_);
}

inline SqliteContext::~SqliteContext() {
    if (conn_) {
        if (sqlite3_close_v2(conn_) == SQLITE_OK) conn_ = nullptr;
    }
}

inline int sqlite_callback(void *NotUsed, int argc, char **argv, char **azColName) {
    int i;
    if (argc > 0) {
        logger_info("sqlite3 exec callback, following key-value:");
        for (i = 0; i < argc; i++) logger_info("%s = %s", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    return 0;
}

inline void SqliteContext::write(const std::string &sql) {
    char *err = nullptr;
    int ret = sqlite3_exec(conn_, sql.c_str(), sqlite_callback, 0, &err);

    if (ret != SQLITE_OK) throw std::runtime_error(errmsg("sqlite write sql failed", ret));
}

inline void SqliteContext::update(const UpdateCond *cond) {
    auto hash = cond->hash_value() + "U";

    return this->template execute<MakeUpdateSql>(
        hash, [](sqlite3_stmt *stmt) { sqlite3_step(stmt); }, cond);
}

inline void SqliteContext::del(int64_t &ret, const ReadCond *cond) {
    auto hash = cond->hash_value() + "D";

    return this->template execute<MakeDeleteSql>(
        hash, [this, &ret](sqlite3_stmt *stmt) { exec_entity(ret, stmt); }, cond);
}

inline void SqliteContext::copy(int64_t &ret, const std::list<JoinReadCond *> &froms,
                                /* const JoinReadCond *to, */ const ReadCond *column,
                                const std::list<ReadCond *> &conds) {
    std::string hash("C");
    for (auto equal : froms) {
        assert(equal);
        hash += equal->hash_value();
    }
    for (auto cond : conds) {
        assert(cond);
        hash += cond->hash_value();
    }
    // hash += to->get_obj_name();
    assert(column);
    hash += column->get_obj_name();

    return this->template execute<MakeQuerySql>(
        hash,
        [&ret](sqlite3_stmt *stmt) {
            sqlite3_step(stmt);
            ret = 0;
        },
        froms, /* to, */ column, conds);
}

template <typename RetObj>
inline void SqliteContext::read(RetObj &ro, const ReadCond *cond) {
    assert(cond);

    auto hash = cond->hash_value();
    return this->template execute<MakeQuerySql>(
        hash, [this, &ro](sqlite3_stmt *stmt) { exec(ro, stmt); }, cond);
}

template <typename RetObj>
inline void SqliteContext::read(RetObj &ro, const std::list<ReadCond *> &conds) {
    logger_debug("context union read, cond size: %ld", conds.size());
    std::string hash;
    for (auto cond : conds) {
        assert(cond);
        hash += cond->hash_value();
    }

    return this->template execute<MakeQuerySql>(
        hash, [this, &ro](sqlite3_stmt *stmt) { exec(ro, stmt); }, conds);
}

template <typename RetObj>
inline void SqliteContext::read(RetObj &ro, const std::list<JoinReadCond *> &equals,
                                const std::list<ReadCond *> &conds) {
    logger_debug("context join read, join cond size: %ld, cond size: %ld", equals.size(),
                 conds.size());
    std::string hash;
    for (auto equal : equals) {
        assert(equal);
        hash += equal->hash_value();
    }

    for (auto cond : conds) {
        assert(cond);
        hash += cond->hash_value();
    }

    ReadCondImpl<RetObj> retcond;
    return this->template execute<MakeQuerySql>(
        hash, [this, &ro](sqlite3_stmt *stmt) { exec(ro, stmt); }, &retcond, equals, conds);
}

template <typename MakeSql, typename... Args>
inline void SqliteContext::execute(const std::string &hash,
                                   const std::function<void(sqlite3_stmt *)> &exe, Args &&...args) {
    auto it = read_stmts_.find(hash);
    if (it == read_stmts_.end()) {
        read_stmts_[hash] = create_stmt<MakeSql>(std::forward<Args>(args)...);
        it = read_stmts_.find(hash);
    }

    auto stmt = it->second;
    BindSqlParam bindparam(stmt);
    try {
        bindparam.bind(std::forward<Args>(args)...);
        exe(stmt);
    } catch (const std::exception &e) {
        sqlite3_finalize(stmt);
        it = read_stmts_.erase(it);
        std::string error = std::string("sqlite bind/exec query sql failed: ") + e.what();
        throw std::runtime_error(error);
    }

    // clear bindings for next loop
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
}

template <typename MakeSql, typename... Args>
inline sqlite3_stmt *SqliteContext::create_stmt(Args &&...args) {
    // 1. make query sql
    MakeSql sqlops;
    auto sql = sqlops.make(std::forward<Args>(args)...);
    sql.append(";");
    logger_info("sqlite create new sql: %s", sql.c_str());

    // 2. create prepared statement
    sqlite3_stmt *stmt = nullptr;
    assert(conn_);
    if (!conn_) throw std::runtime_error("sqlite connection is invalid");
    int ret = sqlite3_prepare_v2(conn_, sql.c_str(), -1, &stmt, nullptr);
    if (ret != SQLITE_OK)
        throw std::runtime_error(errmsg("sqlite create prepared statement failed", ret));

    return stmt;
}

template <typename RetObj, typename std::enable_if<!IsSequenceContainer<RetObj>::value, int>::type>
inline void SqliteContext::exec(RetObj &ro, sqlite3_stmt *stmt) {
    int ret = SQLITE_OK;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        fetch_value(ro, stmt);
        break;
    }

    // if (ret != SQLITE_DONE)
    //     throw std::runtime_error(errmsg("sqlite query one-value failed", ret));
}

template <typename RetObj, typename std::enable_if<IsSequenceContainer<RetObj>::value, int>::type>
inline void SqliteContext::exec(RetObj &ro, sqlite3_stmt *stmt) {
    using Type = typename std::decay<decltype(std::declval<typename RetObj::value_type>())>::type;

    int ret = SQLITE_OK;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        Type obj;
        fetch_value(obj, stmt);
        ro.emplace_back(std::move(obj));
    }

    if (ret != SQLITE_DONE)
        throw std::runtime_error(errmsg("sqlite query multi-value failed: ", ret));
}

inline void SqliteContext::exec_entity(int64_t &r, sqlite3_stmt *stmt) {
    int ret = SQLITE_OK;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        ++r;
    }

    if (ret != SQLITE_DONE)
        throw std::runtime_error(errmsg("sqlite update/delete value failed", ret));
}

template <typename RetObj>
inline void SqliteContext::fetch_value(RetObj &ro, sqlite3_stmt *stmt) {
    FetchSqlResult fetchops(stmt);
    fetchops.fetch(ro);
}

inline std::string SqliteContext::errmsg(const std::string &selfdef, const int ret) {
    return selfdef + ": " + sqlite3_errstr(ret) + "[" + sqlite3_errmsg(conn_) + "]";
}

inline void SqliteContext::set_sqlite_pragma(sqlite3 *conn) {
    // busy_timeout: 1000ms
    static std::map<std::string, std::string> kv = {{"busy_timeout", "1000"},
                                                    {"auto_vacuum", "FULL"},
                                                    {"synchronous", "FULL"},
                                                    {"journal_mode", "WAL"}};

    // set sqlite pragma
    for (auto &e : kv) {
        char *zerrmsg = nullptr;
        std::string sql = "PRAGMA ";
        sql.append(e.first).append(" = ").append(e.second).append(";");

        int ret = sqlite3_exec(conn, sql.c_str(), sqlite_callback, 0, &zerrmsg);
        if (ret != SQLITE_OK) {
            std::string error = std::string("sqlite set pragma failed [key:") + e.first +
                                ", value:" + e.second + "], reason: " + sqlite3_errstr(ret);
            logger_error("%s", error.c_str());
            throw std::runtime_error(error);
        }
        if (zerrmsg) {
            logger_info("sqlite zerrmsg: %s.", zerrmsg);
        }
    }
}

}  // namespace sqlite
}  // namespace yan