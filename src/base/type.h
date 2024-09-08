

#pragma once

#include <list>
#include <string>
#include <type_traits>

#include "com/macro.h"

namespace yan {

/*************************************yan type****************************************/
#define FOR_EACH_INTEGER_TYPE(T) \
    T(int8_t, INT8)              \
    T(int16_t, INT16)            \
    T(int32_t, INT32)            \
    T(int64_t, INT64)            \
    // T(uint8_t, UINT8)          \
    // T(uint16_t, UINT16)        \
    // T(uint32_t, UINT32)        \
    // T(uint64_t, UINT64)
#define FOR_EACH_FLOAT_TYPE(T) \
    T(float, FLOAT)            \
    T(double, DOUBLE)
#define FOR_EACH_DETERMINE_TYPE(T) \
    T(bool, BOOL)                  \
    T(std::string, STRING)
// T(char, CHAR)

#define FOR_EACH_BASIC_TYPE(T) \
    FOR_EACH_INTEGER_TYPE(T)   \
    FOR_EACH_FLOAT_TYPE(T)     \
    FOR_EACH_DETERMINE_TYPE(T)

/**
 * @brief the following implementation is internal
 *
 */
#define FOR_EACH_SPECIAL_TYPE(T) \
    T(int64_t, AUTO_INCREMENT)   \
    T(int64_t, DATE)             \
    T(int64_t, TIMESTAMP)        \
    T(std::string, TEXT)         \
    T(std::string, BLOB)

#define FOR_EACH_INNER_TYPE(T) \
    T(struct, STRUCT)          \
    T(list, LIST)

#define FOR_EACH_TYPE(T)     \
    FOR_EACH_BASIC_TYPE(T)   \
    FOR_EACH_SPECIAL_TYPE(T) \
    FOR_EACH_INNER_TYPE(T)

enum class YanType {
    UNKNOWN = 0,
#define T(t, r) r,
    FOR_EACH_TYPE(T)
#undef T
};

static inline std::string get_yan_type_str(const YanType type) {
    switch (type) {
#define T(a, b)        \
    case YanType::b: { \
        return #b;     \
    }
        FOR_EACH_TYPE(T)
#undef T
        default:
            break;
    }
    return " unknown-yantype ";
}

// please igonre the following transformations, internal use
template <YanType T>
struct YanType2Type {};

#define T(t, r)                       \
    template <>                       \
    struct YanType2Type<YanType::r> { \
        typedef t m_t;                \
    };
FOR_EACH_BASIC_TYPE(T)
FOR_EACH_SPECIAL_TYPE(T)
#undef T

template <typename T>
struct Type2YanType {
    static YanType yan_type() { return YanType::UNKNOWN; }
};

#define T(t, r)                                          \
    template <>                                          \
    struct Type2YanType<t> {                             \
        static YanType yan_type() { return YanType::r; } \
    };
FOR_EACH_BASIC_TYPE(T)
#undef T

template <YanType T>
using YANTYPE = typename YanType2Type<T>::m_t;

/*************************************yan cond type****************************************/
// query condition type
#define FOR_EACH_YAN_COND_TYPE(T) \
    T(AND, "AND")                 \
    T(OR, "OR")                   \
    T(NOT, "NOT")                 \
    T(GROUP, "GROUP")             \
    T(ORDER_ASC, "ORDER_ASC")     \
    T(ORDER_DESC, "ORDER_DESC")

enum class CondType : uint8_t {
    UNKNOWN = 0,
#define T(a, b) a,
    FOR_EACH_YAN_COND_TYPE(T)
#undef T
};

static inline std::string get_yan_cond_str(const CondType condtype) {
    switch (condtype) {
#define T(a, b)         \
    case CondType::a: { \
        return b;       \
    }
        FOR_EACH_YAN_COND_TYPE(T)
#undef T
        default:
            break;
    }
    return " unknown-condition-type ";
}

/*************************************yan ops type****************************************/
#define FOR_EACH_YAN_OPS_TYPE(T)     \
    T(EQUAL, " == ", " != ")         \
    T(EQUAL_LIST, " ", " ")          \
    T(LARGE, " > ", " <= ")          \
    T(LARGE_OR_EQUAL, " >= ", " < ") \
    T(LESS, " < ", " >= ")           \
    T(LESS_OR_EQUAL, " <= ", " > ")

enum class YanOpsType : uint8_t {
    UNKNOWN,
#define T(a, b, c) a,
    FOR_EACH_YAN_OPS_TYPE(T)
#undef T
};

static inline std::string get_yan_ops_str(const YanOpsType opstype, const bool n = false) {
    switch (opstype) {
#define T(a, b, c)        \
    case YanOpsType::a: { \
        if (!n)           \
            return b;     \
        else              \
            return c;     \
    }
        FOR_EACH_YAN_OPS_TYPE(T)
#undef T
        default:
            break;
    }
    return " unknown-operator-type ";
}

/*************************************common type****************************************/
template <typename T>
using YanList = std::list<T>;

}  // namespace yan
