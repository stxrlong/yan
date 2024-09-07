

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <numeric>
#include <thread>
#include <vector>

#include "blockingconcurrentqueue.h"
#include "macro.h"

namespace yan {

class ThreadPool {
    using TElement = std::function<void()> *;
    using TQueue = moodycamel::BlockingConcurrentQueue<TElement>;
    struct Consumer {
        std::thread consumer_;
        std::shared_ptr<TQueue> queue_;
    };

public:
    ThreadPool(const std::string &name, const int size = 1);
    ~ThreadPool();

    template <typename F, typename... Args>
    inline auto insert(F &&f, Args &&...args) ->
        typename std::enable_if<!(std::is_same<typename std::decay<F>::type, std::string>::value ||
                                  std::is_same<typename std::decay<F>::type, const char *>::value ||
                                  std::is_integral<F>::value),
                                bool>::type;

    template <typename F, typename... Args>
    inline bool insert(const std::string &key, F &&f, Args &&...args);

    template <typename F, typename... Args>
    inline bool insert(const uint32_t num, F &&f, Args &&...args);

    template <typename F, typename... Args>
    inline auto submit(F &&f, Args &&...args) ->
        typename std::enable_if<!(std::is_same<typename std::decay<F>::type, std::string>::value ||
                                  std::is_same<typename std::decay<F>::type, const char *>::value ||
                                  std::is_integral<F>::value),
                                future<typename std::result_of<F(Args...)>::type>>::type;

    template <typename F, typename... Args>
    inline auto submit(const std::string &key, F &&f,
                       Args &&...args) -> future<typename std::result_of<F(Args...)>::type>;

    template <typename F, typename... Args>
    inline auto submit(const uint32_t num, F &&f,
                       Args &&...args) -> future<typename std::result_of<F(Args...)>::type>;

    inline int get_consumer_num() { return consumers_.size(); }

    /**
     * @brief one you stop this thread pool, please throw this object
     */
    inline void stop();

    /**
     * @brief there is no need to provide the following func, you can
     * calculate it by satistics the number of insertion and the execuation
     */
    // inline int get_approximate_queue_size();

private:
    std::vector<Consumer> consumers_;
    bool stop_{false};

    std::atomic_int32_t seq_{false};
};

#define STOP_CHECK(flag) \
    if (flag) throw std::runtime_error("this thread pool is stopped");

template <typename F, typename... Args>
inline auto ThreadPool::insert(F &&f, Args &&...args) ->
    typename std::enable_if<!(std::is_same<typename std::decay<F>::type, std::string>::value ||
                              std::is_same<typename std::decay<F>::type, const char *>::value ||
                              std::is_integral<F>::value),
                            bool>::type {
    STOP_CHECK(stop_);

    auto elem =
        new std::function<void()>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    if (!elem) throw std::runtime_error("out of memory");

    auto i = ++seq_ % get_consumer_num();
    return consumers_[i].queue_->enqueue(elem);
}

template <typename F, typename... Args>
inline bool ThreadPool::insert(const std::string &key, F &&f, Args &&...args) {
    STOP_CHECK(stop_);

    auto elem =
        new std::function<void()>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    if (!elem) throw std::runtime_error("out of memory");

    std::hash<std::string> hasher;
    auto i = hasher(key) % get_consumer_num();
    return consumers_[i].queue_->enqueue(elem);
}

template <typename F, typename... Args>
inline bool ThreadPool::insert(const uint32_t num, F &&f, Args &&...args) {
    STOP_CHECK(stop_);

    if (num > consumers_.size()) throw std::runtime_error("invalid consumer number");

    auto elem =
        new std::function<void()>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    if (!elem) throw std::runtime_error("out of memory");

    return consumers_[num].queue_->enqueue(elem);
}

template <typename F, typename... Args>
inline auto ThreadPool::submit(const std::string &key, F &&f, Args &&...args)
    -> future<typename std::result_of<F(Args...)>::type> {
    STOP_CHECK(stop_);

    using Type = typename std::result_of<F(Args...)>::type;
    auto pro = std::make_shared<packaged_task<Type>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    if (!pro) throw std::runtime_error("out of memory");

    auto elem = new std::function<void()>([pro] { (*pro)(); });
    if (!elem) throw std::runtime_error("out of memory");

    std::hash<std::string> hasher;
    auto i = hasher(key) % get_consumer_num();
    if (!consumers_[i].queue_->enqueue(elem)) throw std::runtime_error("enqueue failed");
    return pro->get_future();
}

template <typename F, typename... Args>
inline auto ThreadPool::submit(const uint32_t num, F &&f, Args &&...args)
    -> future<typename std::result_of<F(Args...)>::type> {
    STOP_CHECK(stop_);

    if (num > consumers_.size()) throw std::runtime_error("invalid consumer number");

    using Type = typename std::result_of<F(Args...)>::type;
    auto pro = std::make_shared<packaged_task<Type>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    if (!pro) throw std::runtime_error("out of memory");

    auto elem = new std::function<void()>([pro] { (*pro)(); });
    if (!elem) throw std::runtime_error("out of memory");

    if (!consumers_[num].queue_->enqueue(elem)) throw std::runtime_error("enqueue failed");
    return pro->get_future();
}

template <typename F, typename... Args>
inline auto ThreadPool::submit(F &&f, Args &&...args) ->
    typename std::enable_if<!(std::is_same<typename std::decay<F>::type, std::string>::value ||
                              std::is_same<typename std::decay<F>::type, const char *>::value ||
                              std::is_integral<F>::value),
                            future<typename std::result_of<F(Args...)>::type>>::type {
    STOP_CHECK(stop_);

    using Type = typename std::result_of<F(Args...)>::type;
    auto pro = std::make_shared<packaged_task<Type>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    if (!pro) throw std::runtime_error("out of memory");

    auto elem = new std::function<void()>([pro] { (*pro)(); });
    if (!elem) throw std::runtime_error("out of memory");

    auto i = ++seq_ % get_consumer_num();
    consumers_[i].queue_->enqueue(elem);
    return pro->get_future();
}

#undef STOP_CHECK

}  // namespace yan
