

#include "thread_pool.h"

namespace yan {
ThreadPool::ThreadPool(const std::string &name, const int size) {
    consumers_.resize(size);
    for (auto i = 0; i < size; ++i) {
        auto &consumer = consumers_[i];
        auto &queue = consumer.queue_;
        queue = std::make_shared<TQueue>();

        consumer.consumer_ = std::thread([this, queue, name, i]() {
            assert(queue);
            auto tname = name + "_" + std::to_string(i);
            SET_THREAD_NAME(tname);

            while (true) {
                if (stop_ && queue->size_approx() == 0) return;

                TElement elem;
                if (queue->try_dequeue(elem)) {
                    (*elem)();
                    delete elem;
                } else if (queue->wait_dequeue_timed(elem, 1000)) {
                    (*elem)();
                    delete elem;
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    if (stop_) return;

    stop();
}

void ThreadPool::stop() {
    stop_ = true;

    for (auto &consumer : consumers_) {
        if (consumer.consumer_.joinable()) consumer.consumer_.join();
    }
}

}  // namespace yan
