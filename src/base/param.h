

#pragma once

#include "type.h"

namespace yan {

class ParamBase {
public:
    ParamBase(const char* name, const YanType type, const uint16_t cond, const uint16_t seq)
        : name_(name), type_(type), cond_(cond), seq_(seq) {}
    ParamBase() {}

    inline void prefix(const std::string& prefix) { name_ = prefix + "." + name_; }

    inline const std::string& name() const { return name_; }
    inline const YanType yan_type() const { return static_cast<YanType>(type_); }
    inline const uint16_t yan_cond() const { return cond_; }
    inline const uint16_t yan_seq() const { return seq_; }

protected:
    std::string name_;
    YanType type_;
    uint16_t cond_;
    uint16_t seq_;
};

template <typename T>
class Param : public ParamBase {
public:
    Param(const char* name, const YanType type, const uint16_t cond, const uint16_t seq)
        : ParamBase(name, type, cond, seq) {}
    Param(const char* name, const YanType type, const uint16_t cond, const uint16_t seq, const T& t)
        : ParamBase(name, type, cond, seq) {
        t_ = t;
    }
    Param(const char* name, const YanType type, const uint16_t cond, const uint16_t seq, T&& t)
        : ParamBase(name, type, cond, seq) {
        t_ = std::forward<T>(t);
    }
    Param(const Param<T>& r) {
        this->type_ = r.type_;
        this->cond_ = r.cond_;
        this->seq_ = r.seq_;
        this->t_ = r.t_;
    }

    inline const T& get() const { return t_; }

private:
    T t_;
};

}  // namespace yan
