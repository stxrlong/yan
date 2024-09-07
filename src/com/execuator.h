

#pragma once

#include "thread_pool.h"

namespace yan {

class Execuator : public ThreadPool {
public:
    Execuator(const std::string &name, const int size = 1) : ThreadPool(name, size) {}
    ~Execuator() = default;
};

}  // namespace yan
