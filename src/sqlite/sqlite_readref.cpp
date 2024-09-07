

#include "sqlite_readref.h"

namespace yan {
namespace sqlite {
/***********************************create table sql**************************************/
std::string MakeTableSql::create(const std::string &table) {
    std::string tablesql;

    tablesql.append("CREATE TABLE IF NOT EXISTS ").append(table).append(" ");

    tablesql.append("(").append(sqlctx_.fields_).append(") ");

    return tablesql;
}

int MakeTableSql::ReadMemRef::append(const std::string &name, const YanType type,
                                     const bool canbenull, const int32_t length) {
    if (sqlctx_.append_first_)
        sqlctx_.append_first_ = false;
    else
        sqlctx_.fields_.append(", ");

    sqlctx_.fields_.append(name);

    switch (type) {
        case YanType::AUTO_INCREMENT: {
            sqlctx_.fields_.append(" INTEGER PRIMARY_KEY AUTO_INCREMENT");
            return 0;
        }
#define T(t, f) case YanType::f:
            FOR_EACH_INTEGER_TYPE(T)
#undef T
            {
                sqlctx_.fields_.append(" INT");
            }
            break;
        case YanType::STRING: {
            if (length <= 0)
                throw std::runtime_error(
                    std::string("sqlite's field length with char type should be large than ") +
                    std::to_string(LEAST_TABLE_FIELD_CHAR_LENGTH));
            sqlctx_.fields_.append(" CHAR(").append(std::to_string(length)).append(")");
        } break;
        case YanType::DATE: {
            sqlctx_.fields_.append(" INT");
        } break;
        default:
            throw std::runtime_error(std::string("unsupports the field type [") +
                                     get_yan_type_str(type) + "]");
    }

    if (!canbenull) sqlctx_.fields_.append(" NOT NULL");

    return 0;
}
/*************************************MakeInsertSql****************************************/
std::string MakeInsertSql::create(const std::string &name) {
    std::string insertsql;

    insertsql.append("INSERT INTO ").append(name).append(" ");

    insertsql.append("(").append(sqlctx_.fields_).append(") ");

    insertsql.append("VALUES (").append(sqlctx_.values_).append(")");

    return insertsql;
}

template <>
void MakeInsertSql::ReadMemRef::append_value(const std::function<const std::string &()> &cb) {
    if (sqlctx_.onlyone_) sqlctx_.fields_.append(name_);
    sqlctx_.values_.append("'").append(cb()).append("'");
}

#define APPEND_INSERT_SQL(T)                                                             \
    template <>                                                                          \
    void MakeInsertSql::ReadMemRef::append_value(const std::function<const T &()> &cb) { \
        if (sqlctx_.onlyone_) sqlctx_.fields_.append(name_);                             \
        sqlctx_.values_.append(std::to_string(cb()));                                    \
    }

#define T(a, b) APPEND_INSERT_SQL(a)

FOR_EACH_INTEGER_TYPE(T)
FOR_EACH_FLOAT_TYPE(T)
#undef T

APPEND_INSERT_SQL(bool)

#undef APPEND_INSERT_SQL

/*************************************MakeQuerySql****************************************/
std::string MakeQuerySql::make(const ReadCond *cond) {
    auto &sql = sqlctx_.sql_;
    sql.append("SELECT ");

    int ret = 0;
    if (cond->ret_equal_to_cond()) {
        sql.append("*");
    } else {
        ret = cond->get_read_members([this](const std::string &table, const std::string &name) {
            if (!sqlctx_.append_ret_first) {
                sqlctx_.sql_.append(", ");
            } else {
                sqlctx_.append_ret_first = false;
                if (table == COUNT_FIELD) {
                    sqlctx_.sql_.append("COUNT(").append(name).append(")");
                    return 0;
                }
            }

            if (!table.empty()) sqlctx_.sql_.append(table).append(".");
            sqlctx_.sql_.append(name);
            return 0;
        });
        if (ret < 0)
            throw std::runtime_error("sqlite get return object's members for query sql failed: " +
                                     std::to_string(ret));
    }

    sql.append(" FROM ").append(cond->get_obj_name());

    if (!cond->has_cond()) return sql;

    sql.append(" WHERE ");

    if ((ret = queryops_.append_cond(sqlctx_, *cond)) < 0)
        throw std::runtime_error("sqlite append condition for query sql failed: " +
                                 std::to_string(ret));

    return sql;
}

std::string MakeQuerySql::make(const std::list<ReadCond *> &conds) {
    if (conds.size() == 0) throw std::runtime_error("no condition for union read");

    auto count = 0;
    for (auto cond : conds) {
        if (++count > 1) sqlctx_.sql_.append(" UNION ");

        sqlctx_.append_ret_first = true;
        sqlctx_.append_cond_first = true;
        make(cond);
    }

    return sqlctx_.sql_;
}

std::string MakeQuerySql::make(const ReadCond *retcond, const std::list<JoinReadCond *> &equals,
                               const std::list<ReadCond *> &conds) {
    if (equals.size() == 0) throw std::runtime_error("no condition for join read");

    auto &sql = sqlctx_.sql_;
    sql.append("SELECT ");
    // 1. set return obj
    int ret = retcond->get_read_members([this](const std::string &table, const std::string &name) {
        if (!sqlctx_.append_ret_first) {
            sqlctx_.sql_.append(", ");
        } else {
            sqlctx_.append_ret_first = false;
        }

        if (!table.empty()) sqlctx_.sql_.append(table).append(".");
        sqlctx_.sql_.append(name);
        return 0;
    });
    if (ret < 0)
        throw std::runtime_error("sqlite get return object's members for join read sql failed: " +
                                 std::to_string(ret));

    return left_join(equals, conds);
}

std::string MakeQuerySql::make(const std::list<JoinReadCond *> &froms, /* const JoinReadCond *to, */
                               const ReadCond *column, const std::list<ReadCond *> &conds) {
    if (froms.size() != 1) throw std::runtime_error("unknow copy from table");

    auto &name = froms.front()->get_obj_name();

    std::string sql;
    sql.append("INSERT INTO ").append(column->get_obj_name());

    // 1. set copy obj
    int ret = column->get_read_members([this](const std::string &table, const std::string &name) {
        if (!sqlctx_.append_ret_first) {
            sqlctx_.sql_.append(", ");
        } else {
            sqlctx_.append_ret_first = false;
        }

        if (!table.empty()) sqlctx_.sql_.append(table).append(".");
        sqlctx_.sql_.append(name);
        return 0;
    });
    if (ret < 0)
        throw std::runtime_error("sqlite get column members for copy sql failed: " +
                                 std::to_string(ret));

    sql.append(" (").append(sqlctx_.sql_).append(") SELECT ").append(sqlctx_.sql_);
    sqlctx_.sql_.assign(std::move(sql)).append(" FROM ").append(name);

    // 2. set condition
    if (conds.size() == 0) return sqlctx_.sql_;

    sqlctx_.sql_.append(" WHERE ");
    for (auto cond : conds) {
        if ((ret = queryops_.append_cond(sqlctx_, *cond)) < 0)
            throw std::runtime_error("sqlite append condition for query sql failed: " +
                                     std::to_string(ret));
    }

    return sqlctx_.sql_;
}

std::string MakeQuerySql::left_join(const std::list<JoinReadCond *> &equals,
                                    const std::list<ReadCond *> &conds) {
    auto &sql = sqlctx_.sql_;
    // 1. set join info
    auto count = 0;
    for (auto equal : equals) {
        assert(equal);
        if (++count == 1)
            sql.append(" FROM ").append(equal->get_obj_name());
        else
            sql.append(" LEFT JOIN ").append(equal->get_obj_name()).append(" ON ");

        auto c = 0;
        auto &es = equal->get_equals();
        for (auto &e : es) {
            if (++c == 1)
                sql.append(std::move(e));
            else
                sql.append(" AND ").append(std::move(e));
        }
    }

    // 2. set condition
    if (conds.size() > 0) sql.append(" WHERE ");
    int ret = 0;
    for (auto cond : conds) {
        if ((ret = queryops_.append_cond(sqlctx_, *cond)) < 0)
            throw std::runtime_error("sqlite append condition for query sql failed: " +
                                     std::to_string(ret));
    }

    return sql;
}

int MakeQuerySql::ReadMemRef::append_cond(const CondType condtype, const YanOpsType opstype) {
    switch (condtype) {
        case CondType::AND: {
            if (sqlctx_.append_cond_first)
                sqlctx_.append_cond_first = false;
            else
                sqlctx_.sql_.append(" AND ");
            sqlctx_.sql_.append(name_).append(get_yan_ops_str(opstype)).append("?");
        } break;
        case CondType::OR: {
            if (sqlctx_.append_cond_first)
                sqlctx_.append_cond_first = false;
            else
                sqlctx_.sql_.append(" OR ");
            sqlctx_.sql_.append(name_).append(get_yan_ops_str(opstype)).append("?");
        } break;
        case CondType::NOT: {
            if (sqlctx_.append_cond_first)
                sqlctx_.append_cond_first = false;
            else
                sqlctx_.sql_.append(" AND ");
            sqlctx_.sql_.append(name_).append(get_yan_ops_str(opstype, true)).append("?");
        } break;
        case CondType::GROUP: {
            if (sqlctx_.append_cond_first)
                throw std::runtime_error("sqlite's first condition should be pointed to a field");
            sqlctx_.sql_.append(" GROUP BY ").append(name_);
        } break;
        case CondType::ORDER_ASC: {
            if (sqlctx_.append_cond_first)
                throw std::runtime_error("sqlite's first condition should be pointed to a field");
            sqlctx_.sql_.append(" ORDER BY ").append(name_);
        } break;
        case CondType::ORDER_DESC: {
            if (sqlctx_.append_cond_first)
                throw std::runtime_error("sqlite's first condition should be pointed to a field");
            sqlctx_.sql_.append(" ORDER BY ").append(name_).append(" DESC ");
        } break;
        default: {
            throw std::runtime_error("unknown condition type: " +
                                     std::to_string(static_cast<int>(condtype)));
        }
    }

    return 0;
}

/*************************************MakeUpdateSql***************************************/
std::string MakeUpdateSql::make(const UpdateCond *cond) {
    auto &updatesql = fieldctx_.fields_;

    updatesql.append("UPDATE ").append(cond->get_obj_name()).append(" SET ");

    int ret = updateops_.append_value(fieldctx_, *cond);
    if (ret < 0) throw std::runtime_error("invalid updating value when doing update operation");

    if (!cond->has_cond()) return updatesql;

    updatesql.append(" WHERE ");
    sqlctx_.sql_ = std::move(updatesql);

    if ((ret = queryops_.append_cond(sqlctx_, *cond)) < 0)
        throw std::runtime_error("sqlite append condition for update sql failed: " +
                                 std::to_string(ret));

    return sqlctx_.sql_;
}

/*************************************MakeDeleteSql***************************************/
std::string MakeDeleteSql::make(const ReadCond *cond) {
    auto &deletesql = sqlctx_.sql_;

    deletesql.append("DELETE FROM ").append(cond->get_obj_name());

    if (!cond->has_cond()) return deletesql;

    deletesql.append(" WHERE ");

    int ret = queryops_.append_cond(sqlctx_, *cond);
    if (ret < 0)
        throw std::runtime_error("sqlite append condition for delete sql failed: " +
                                 std::to_string(ret));

    return deletesql;
}

}  // namespace sqlite
}  // namespace yan
