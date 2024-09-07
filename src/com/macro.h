

#pragma once

#include <boost/thread/executors/basic_thread_pool.hpp>
#include <boost/thread/future.hpp>
#include <string>

namespace yan {
/*************************************macro os****************************************/
#ifdef __APPLE__
#define SET_THREAD_NAME(name) pthread_setname_np(name.c_str());
#else
#define SET_THREAD_NAME(name) pthread_setname_np(pthread_self(), name.c_str());
#endif

/*************************************macro expand****************************************/
#define PRIVATE_ARGS_GLUE(x, y) x y
#define PRIVATE_MACRO_VAR_ARGS_IMPL_COUNT(_1, _2, _3, _4, _5, _6, _7, _8, _9, N, ...) N
#define PRIVATE_MACRO_VAR_ARGS_IMPL(args) PRIVATE_MACRO_VAR_ARGS_IMPL_COUNT args
#define COUNT_MACRO_VAR_ARGS(...) \
    PRIVATE_MACRO_VAR_ARGS_IMPL((__VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define PRIVATE_MACRO_CHOOSE_HELPER2(M, count) M##count
#define PRIVATE_MACRO_CHOOSE_HELPER1(M, count) PRIVATE_MACRO_CHOOSE_HELPER2(M, count)
#define PRIVATE_MACRO_CHOOSE_HELPER(M, count) PRIVATE_MACRO_CHOOSE_HELPER1(M, count)

#define VARIADIC_MACRO_INIT(M, ...)                                                      \
    PRIVATE_ARGS_GLUE(PRIVATE_MACRO_CHOOSE_HELPER(M, COUNT_MACRO_VAR_ARGS(__VA_ARGS__)), \
                      (__VA_ARGS__))

/*************************************stl type****************************************/
template <typename...>
using Void = void;

template <typename T, typename = Void<>>
struct HasPushBack : std::false_type {};

template <typename T>
struct HasPushBack<T, typename std::enable_if<std::is_void<decltype(std::declval<T>().push_back(
                          std::declval<typename T::value_type>()))>::value>::type>
    : std::true_type {};

template <typename T, typename = Void<>>
struct HasInsert : std::false_type {};

template <typename T>
struct HasInsert<T,
                 typename std::enable_if<std::is_same<
                     decltype(std::declval<T>().insert(std::declval<typename T::const_iterator>(),
                                                       std::declval<typename T::value_type>())),
                     typename T::iterator>::value>::type> : std::true_type {};

template <typename T>
struct IsSequenceContainer
    : std::integral_constant<bool,
                             HasPushBack<T>::value &&
                                 !std::is_same<typename std::decay<T>::type, std::string>::value> {
};

template <typename T>
struct IsAssociativeContainer
    : std::integral_constant<bool, HasInsert<T>::value && !HasPushBack<T>::value> {};

template <typename Value, typename T = Void<>>
struct SequenceValueType {
    using type = Value;
};

template <typename Value>
struct SequenceValueType<Value, typename std::enable_if<IsSequenceContainer<Value>::value>::type> {
    using type = typename std::decay<decltype(std::declval<typename Value::value_type>())>::type;
};

/*************************************boost****************************************/
template <typename T>
using future = boost::future<T>;
template <typename T>
using promise = boost::promise<T>;
template <typename T>
using packaged_task = boost::packaged_task<T>;

template <typename T, typename E>
future<T> make_yan_exception_future(E e) {
    return boost::make_exceptional_future<T>(e);
}

}  // namespace yan
