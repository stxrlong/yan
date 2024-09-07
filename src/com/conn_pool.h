

#pragma once

#include <atomic>
#include <memory>

#include "logger.h"

namespace yan {

/**
 * @brief this is a lock-free connection pool based on the finite thread usage
 *
 * @tparam Connection
 */
template <typename Connection>
class ConnectionPool {
    using ConnectionPtr = std::shared_ptr<Connection>;

public:
    ConnectionPool(const std::string& url) : url_(url) {
        // lazily create the connnection
        spin_lock_.clear(std::memory_order_release);
    }
    ~ConnectionPool() = default;

    ConnectionPtr get() {
        while (true) {
            if (!spin_lock_.test_and_set(std::memory_order_acquire)) {
                ConnectionPtr conn;
                if (conns_.size() > 0) {
                    conn = std::move(conns_.front());
                    conns_.pop_front();
                } else {
                    try {
                        conn = std::make_shared<Connection>(url_);
                        logger_debug("create new conncetion ok, now is the %d connection",
                                     ++used_conns_);
                    } catch (const std::exception& e) {
                        logger_error("create connection failed: %s", e.what());

                        spin_lock_.clear(std::memory_order_release);
                        throw std::runtime_error(e.what());
                    }
                }

                spin_lock_.clear(std::memory_order_release);
                return conn;
            }
        }
    }

    void release(ConnectionPtr& conn) {
        if (!conn) return;

        while (true) {
            if (!spin_lock_.test_and_set(std::memory_order_acquire)) {
                conns_.emplace_back(std::move(conn));

                spin_lock_.clear(std::memory_order_release);
                break;
            }
        }
    }

private:
    std::string url_;

    std::atomic_flag spin_lock_;
    std::deque<ConnectionPtr> conns_;

    int32_t used_conns_ = 0;
};

}  // namespace yan
