

#pragma once

#include <functional>
#include <iostream>
#include <list>
#include <set>
#include <unordered_map>

#include "param.h"
#include "type.h"

namespace yan {

using namespace std::placeholders;

// the following implementation refers from the kernel usage of `container_of`
#define __OFFSET__(obj, member) ((size_t)(&((obj *)0)->member))

using ParamList = std::list<std::shared_ptr<ParamBase>>;
using ParamListPtr = std::shared_ptr<ParamList>;
#define __YAN_STRUCT__(OBJ, args...)                                                           \
    class OBJ {                                                                                \
    public:                                                                                    \
        OBJ(const std::function<void(const uint16_t)> &deepcb) : deepcb_(deepcb) {}            \
        OBJ() {}                                                                               \
        ~OBJ() = default;                                                                      \
                                                                                               \
        static YanStruct *get_yan_struct() {                                                   \
            static YanStruct __yan_struct_##OBJ##__(                                           \
                YAN_STRUCT_CONSTRUCTOR(#OBJ, __OFFSET__(OBJ, conds_), __OFFSET__(OBJ, hashv_), \
                                       __OFFSET__(OBJ, seqset_), ##args));                     \
            return &__yan_struct_##OBJ##__;                                                    \
        }                                                                                      \
                                                                                               \
    private:                                                                                   \
        std::set<uint16_t> seqset_;                                                            \
        ParamListPtr conds_ = nullptr;                                                         \
        std::string hashv_ = "";                                                               \
        std::function<void(const uint16_t)> deepcb_ = nullptr;                                 \
                                                                                               \
        inline void append(const uint16_t seq) { seqset_.insert(seq); }                        \
        template <typename T>                                                                  \
        inline void append(const char *name, const YanType type, const uint16_t cond,          \
                           const uint16_t seq, T &&t) {                                        \
            if (!conds_) conds_ = std::make_shared<ParamList>();                               \
            if (deepcb_ && conds_->size() == 0) deepcb_(seq);                                  \
            hashv_.append((char *)(&seq), 2) /* append((char *)(&cond), 2) */;                 \
            conds_->emplace_back(                                                              \
                std::make_shared<Param<T>>(name, type, cond, seq, std::forward<T>(t)));        \
        }                                                                                      \
        template <typename T>                                                                  \
        inline void append(const char *name, const YanType type, const uint16_t cond,          \
                           const uint16_t seq, const T &t) {                                   \
            if (!conds_) conds_ = std::make_shared<ParamList>();                               \
            if (deepcb_ && conds_->size() == 0) deepcb_(seq);                                  \
            hashv_.append((char *)(&seq), 2).append((char *)(&cond), 2);                       \
            conds_->emplace_back(std::make_shared<Param<T>>(name, type, cond, seq, t));        \
        }

#define __SET_MEM_COND_TYPE__(YTYPE, MEM, opstype, value)                                   \
    if (cond_ == 0)                                                                         \
        throw std::runtime_error("please specify the condition type, AND()/OR()/IN() ..."); \
    uint16_t c = (((uint16_t)cond_) << 8) | static_cast<uint16_t>(opstype);                 \
    obj_->append(#MEM, YTYPE, c, obj_->__yan_member_seq_##MEM##__(), value);                \
    cond_ = 0;
#define __GET_MEM_OPS_TYPE__(x) (x & 0x0FF)
#define __GET_MEM_COND_TYPE__(x) ((x & 0xFF00) >> 8)

#define __MEM_COND_TYPE__(x) static_cast<CondType>(x & 0x00FF)

#define __YAN_MEMBER__(OBJ, MEM, YTYPE, args...)                                                  \
private:                                                                                          \
    uint16_t __yan_member_seq_##MEM##__(const uint16_t seq = 0) {                                 \
        static uint16_t s = seq;                                                                  \
        return s;                                                                                 \
    }                                                                                             \
    class __yan_member_register_##MEM##__;                                                        \
    friend class __yan_member_##MEM##__;                                                          \
                                                                                                  \
public:                                                                                           \
    class __yan_member_##MEM##__ {                                                                \
        friend __yan_member_register_##MEM##__;                                                   \
        static const YanType yantype_ = YTYPE;                                                    \
                                                                                                  \
    public:                                                                                       \
        using TYPE = YANTYPE<YTYPE>;                                                              \
                                                                                                  \
    public:                                                                                       \
        explicit __yan_member_##MEM##__(OBJ *obj) : obj_(obj) {}                                  \
        __yan_member_##MEM##__() {}                                                               \
                                                                                                  \
        inline operator TYPE() { return value_; }                                                 \
        inline const TYPE &VALUE() const { return value_; }                                       \
        inline const bool FLAG() const { return flag_; }                                          \
                                                                                                  \
        inline __yan_member_##MEM##__ &AND() {                                                    \
            cond_ = static_cast<uint8_t>(CondType::AND);                                          \
            return *this;                                                                         \
        }                                                                                         \
        inline __yan_member_##MEM##__ &OR() {                                                     \
            cond_ = static_cast<uint8_t>(CondType::OR);                                           \
            return *this;                                                                         \
        }                                                                                         \
        inline __yan_member_##MEM##__ &NOT() {                                                    \
            cond_ = static_cast<uint8_t>(CondType::NOT);                                          \
            return *this;                                                                         \
        }                                                                                         \
        inline void GROUP() {                                                                     \
            cond_ = static_cast<uint8_t>(CondType::GROUP);                                        \
            __SET_MEM_COND_TYPE__(YTYPE, MEM, YanOpsType::EQUAL, TYPE{});                         \
        }                                                                                         \
        inline void ORDER_ASC() {                                                                 \
            cond_ = static_cast<uint8_t>(CondType::ORDER_ASC);                                    \
            __SET_MEM_COND_TYPE__(YTYPE, MEM, YanOpsType::EQUAL, TYPE{})                          \
        }                                                                                         \
        inline void ORDER_DESC() {                                                                \
            cond_ = static_cast<uint8_t>(CondType::ORDER_DESC);                                   \
            __SET_MEM_COND_TYPE__(YTYPE, MEM, YanOpsType::EQUAL, TYPE{})                          \
        }                                                                                         \
                                                                                                  \
        inline void operator=(const TYPE &r) {                                                    \
            if (cond_ > 0) {                                                                      \
                throw std::runtime_error(                                                         \
                    "You need to use conditional statements, such as ==, >, <=, instead of "      \
                    "assignment statements, when you specify the conditon type: " +               \
                    get_yan_cond_str(static_cast<CondType>(cond_)));                              \
            } else {                                                                              \
                if (yantype_ == YanType::AUTO_INCREMENT) {                                        \
                    std::string error =                                                           \
                        std::string("auto increment member cannot be assigned with value [") +    \
                        #OBJ + "::" #MEM + "]";                                                   \
                    throw std::runtime_error(error);                                              \
                }                                                                                 \
                value_ = r;                                                                       \
                if (!flag_) {                                                                     \
                    obj_->append(obj_->__yan_member_seq_##MEM##__());                             \
                    flag_ = true;                                                                 \
                }                                                                                 \
            }                                                                                     \
        }                                                                                         \
        inline void operator=(TYPE &&r) {                                                         \
            if (cond_ > 0) {                                                                      \
                throw std::runtime_error(                                                         \
                    "You need to use conditional statements, such as ==, >, <=, instead of "      \
                    "assignment statements, when you specify the conditon type: " +               \
                    get_yan_cond_str(static_cast<CondType>(cond_)));                              \
            } else {                                                                              \
                if (yantype_ == YanType::AUTO_INCREMENT) {                                        \
                    std::string error =                                                           \
                        std::string("auto increment member cannot be assigned with value [") +    \
                        #OBJ + "::" + #MEM + "]";                                                 \
                    throw std::runtime_error(error);                                              \
                }                                                                                 \
                value_ = std::forward<TYPE>(r);                                                   \
                if (!flag_) {                                                                     \
                    obj_->append(obj_->__yan_member_seq_##MEM##__());                             \
                    flag_ = true;                                                                 \
                }                                                                                 \
            }                                                                                     \
        }                                                                                         \
        inline void operator==(TYPE &&r) {                                                        \
            __SET_MEM_COND_TYPE__(YTYPE, MEM, YanOpsType::EQUAL, std::forward<TYPE>(r));          \
        }                                                                                         \
        inline void operator==(const TYPE &r) {                                                   \
            __SET_MEM_COND_TYPE__(YTYPE, MEM, YanOpsType::EQUAL, r);                              \
        }                                                                                         \
        inline void operator==(std::initializer_list<TYPE> rlist) {                               \
            __SET_MEM_COND_TYPE__(YTYPE, MEM, YanOpsType::EQUAL_LIST,                             \
                                  std::forward<std::list<TYPE>>(rlist));                          \
        }                                                                                         \
        inline void operator>(const TYPE &r) {                                                    \
            __SET_MEM_COND_TYPE__(YTYPE, MEM, YanOpsType::LARGE, r);                              \
        }                                                                                         \
        inline void operator>=(const TYPE &r) {                                                   \
            __SET_MEM_COND_TYPE__(YTYPE, MEM, YanOpsType::LARGE_OR_EQUAL, r);                     \
        }                                                                                         \
        inline void operator<(const TYPE &r) {                                                    \
            __SET_MEM_COND_TYPE__(YTYPE, MEM, YanOpsType::LESS, r);                               \
        }                                                                                         \
        inline void operator<=(const TYPE &r) {                                                   \
            __SET_MEM_COND_TYPE__(YTYPE, MEM, YanOpsType::LESS_OR_EQUAL, r);                      \
        }                                                                                         \
                                                                                                  \
    private:                                                                                      \
        uint8_t cond_{0};                                                                         \
        bool flag_{false};                                                                        \
        TYPE value_;                                                                              \
        OBJ *obj_ = nullptr;                                                                      \
    } MEM{this};                                                                                  \
                                                                                                  \
private:                                                                                          \
    class __yan_member_register_##MEM##__ {                                                       \
    public:                                                                                       \
        __yan_member_register_##MEM##__(const std::function<void(uint16_t)> &cb) {                \
            static MemberInfo *info =                                                             \
                MEMBER_INFO_CONSTRUCTOR(#MEM, __OFFSET__(OBJ, MEM), YTYPE, ##args);               \
            static __yan_member_register__ __yan_reg_##MEM##__(                                   \
                OBJ::get_yan_struct(), info,                                                      \
                __OFFSET__(OBJ::__yan_member_##MEM##__, OBJ::__yan_member_##MEM##__::value_),     \
                __OFFSET__(OBJ::__yan_member_##MEM##__, OBJ::__yan_member_##MEM##__::flag_), cb); \
        }                                                                                         \
    } __yan_member_reg_##MEM##__{[this](uint16_t seq) { this->__yan_member_seq_##MEM##__(seq); }};

#define __YAN_STRUCT_END__ \
    }                      \
    ;

/***********************************YAN TMP STRUCT****************************************/
#define __YAN_TMP_STRUCT__(OBJ)                                     \
    class OBJ {                                                     \
    public:                                                         \
        OBJ() = default;                                            \
        ~OBJ() = default;                                           \
                                                                    \
        static YanStruct *get_yan_struct() {                        \
            static YanStruct __yan_struct_##OBJ##__(#OBJ, 0, 0, 0); \
            return &__yan_struct_##OBJ##__;                         \
        }

#define __YAN_TMP_MEMBER__(OBJ, MEM, FROM)                                                    \
private:                                                                                      \
    class __yan_tmp_member_register_##MEM##__;                                                \
                                                                                              \
public:                                                                                       \
    class __yan_member_##MEM##__ {                                                            \
        friend __yan_tmp_member_register_##MEM##__;                                           \
        using TYPE = typename std::decay<                                                     \
            decltype(std::declval<typename FROM::__yan_member_##MEM##__::TYPE>())>::type;     \
                                                                                              \
    public:                                                                                   \
        inline const TYPE &VALUE() const { return value_; }                                   \
        inline const bool FLAG() const { return flag_; }                                      \
                                                                                              \
    private:                                                                                  \
        bool flag_{false};                                                                    \
        TYPE value_;                                                                          \
    } MEM;                                                                                    \
                                                                                              \
private:                                                                                      \
    class __yan_tmp_member_register_##MEM##__ {                                               \
    public:                                                                                   \
        __yan_tmp_member_register_##MEM##__() {                                               \
            static __yan_tmp_member_register__ __yan_reg_##MEM##__(                           \
                OBJ::get_yan_struct(), FROM::get_yan_struct(), #MEM, __OFFSET__(OBJ, MEM),    \
                __OFFSET__(OBJ::__yan_member_##MEM##__, OBJ::__yan_member_##MEM##__::value_), \
                __OFFSET__(OBJ::__yan_member_##MEM##__, OBJ::__yan_member_##MEM##__::flag_)); \
        }                                                                                     \
    } __yan_tmp_member_reg_##MEM##__;

/***********************************YAN FIELD BRIDGE****************************************/
#define __YAN_FIELD_BRIDGE__(OBJ1, FIELD1, OBJ2, FIELD2) \
    field_bridge(OBJ1::get_yan_struct(), #FIELD1, OBJ2::get_yan_struct(), #FIELD2)

/***********************************YAN STRUCT OPERATOR*************************************/
class YanMember;
class YanListMember;
class YanStruct;

const int LEAST_TABLE_FIELD_CHAR_LENGTH = 32;
struct MemberInfo {
    const YanType type_;

    const std::string name_;
    const std::string alias_;

    const size_t offset_;

    YanMember *member_ = nullptr;
    YanListMember *list_member_ = nullptr;
    YanStruct *struct_member_ = nullptr;

    MemberInfo(const std::string &n, const size_t o, const YanType t)
        : name_(n), offset_(o), type_(t) {}

    MemberInfo(const std::string &n, const size_t o, const YanType t, const std::string &a,
               const int length, const bool canbenull)
        : name_(n), offset_(o), type_(t), alias_(a), can_be_null_(canbenull), length_(length) {}
    MemberInfo(const std::string &n, const size_t o, const YanType t, const std::string &a,
               const bool canbenull)
        : name_(n), offset_(o), type_(t), alias_(a), can_be_null_(canbenull) {}
    MemberInfo(const std::string &n, const size_t o, const YanType t, const std::string &a,
               const int length)
        : name_(n), offset_(o), type_(t), alias_(a), length_(length) {}
    MemberInfo(const std::string &n, const size_t o, const YanType t, const std::string &a)
        : name_(n), offset_(o), type_(t), alias_(a) {}

    MemberInfo(const std::string &n, const size_t o, const YanType t, const int length,
               const bool canbenull)
        : name_(n), offset_(o), type_(t), can_be_null_(canbenull), length_(length) {}
    MemberInfo(const std::string &n, const size_t o, const YanType t, const bool canbenull)
        : name_(n), offset_(o), type_(t), can_be_null_(canbenull) {}
    MemberInfo(const std::string &n, const size_t o, const YanType t, const int length)
        : name_(n), offset_(o), type_(t), length_(length) {}

    MemberInfo(YanStruct *from, const std::string &n, const size_t o, const YanType t,
               const std::string &a)
        : from_(from), name_(n), offset_(o), type_(t), alias_(a) {}

    void set_value(const std::string &n, const size_t o, const YanType t) {}
    void set_value(const std::string &n, const size_t o, const YanType t, const std::string &a) {}
    void set_value(const std::string &n, const size_t o, const YanType t, const bool canbenull) {}

    inline const std::string &get_name() const {
        if (alias_.empty()) {
            return name_;
        } else {
            return alias_;
        }
    }

    // extra info for creating a table in those structure db
    const YanStruct *from_ = nullptr;
    // VCHAR size, useless when the YanType is not equal to STRING
    const int length_ = LEAST_TABLE_FIELD_CHAR_LENGTH;
    const bool can_be_null_ = false;
};

static MemberInfo *CREATE_MEMBER_INFO3(const std::string &n, const size_t o, const YanType t) {
    return new MemberInfo(n, o, t);
}
static MemberInfo *CREATE_MEMBER_INFO4(const std::string &n, const size_t o, const YanType t,
                                       const std::string &a) {
    return new MemberInfo(n, o, t, a);
}
static MemberInfo *CREATE_MEMBER_INFO4(const std::string &n, const size_t o, const YanType t,
                                       const bool canbenull) {
    return new MemberInfo(n, o, t, canbenull);
}
static MemberInfo *CREATE_MEMBER_INFO4(const std::string &n, const size_t o, const YanType t,
                                       const int length) {
    return new MemberInfo(n, o, t, length);
}
static MemberInfo *CREATE_MEMBER_INFO5(const std::string &n, const size_t o, const YanType t,
                                       const std::string &a, const bool canbenull) {
    return new MemberInfo(n, o, t, a, canbenull);
}
static MemberInfo *CREATE_MEMBER_INFO5(const std::string &n, const size_t o, const YanType t,
                                       const std::string &a, const int length) {
    return new MemberInfo(n, o, t, a, length);
}
static MemberInfo *CREATE_MEMBER_INFO5(const std::string &n, const size_t o, const YanType t,
                                       const int length, const bool canbenull) {
    return new MemberInfo(n, o, t, length, canbenull);
}
static MemberInfo *CREATE_MEMBER_INFO6(const std::string &n, const size_t o, const YanType t,
                                       const std::string &a, const int length,
                                       const bool canbenull) {
    return new MemberInfo(n, o, t, a, length, canbenull);
}

#define CREATE_MEMBER_INFO4(n, o, t, a) CREATE_MEMBER_INFO4(n, o, t, std::string(#a))
#define CREATE_MEMBER_INFO5(n, o, t, a, five) CREATE_MEMBER_INFO5(n, o, t, std::string(#a), five)
#define CREATE_MEMBER_INFO6(n, o, t, a, five, six) \
    CREATE_MEMBER_INFO6(n, o, t, std::string(#a), five, six)
#define MEMBER_INFO_CONSTRUCTOR(args...) VARIADIC_MACRO_INIT(CREATE_MEMBER_INFO, ##args)
#define CREATE_YAN_STRUCT4(n, c, h, s) n, c, h, s
#define CREATE_YAN_STRUCT5(n, c, h, s, a) n, c, h, s, std::string(#a)
#define YAN_STRUCT_CONSTRUCTOR(args...) VARIADIC_MACRO_INIT(CREATE_YAN_STRUCT, ##args)

class YanStruct {
public:
    YanStruct(const std::string &name, const size_t coffset, const size_t hoffset,
              const size_t soffset)
        : name_(name), coffset_(coffset), hoffset_(hoffset), soffset_(soffset) {}
    YanStruct(const std::string &name, const size_t coffset, const size_t hoffset,
              const size_t soffset, const std::string &alias)
        : name_(name), coffset_(coffset), hoffset_(hoffset), soffset_(soffset), alias_(alias) {}

    inline const std::string &get_origin_name() const { return name_; }
    inline const std::string &get_name() const {
        if (alias_.empty()) {
            return name_;
        } else {
            return alias_;
        }
    }

    inline uint16_t add(MemberInfo *member) {
        assert(member);
        auto &name = member->name_;
        auto it = ms_.find(name);
        if (it != ms_.end()) {
            throw std::runtime_error(std::string("you have defined same member in struct") + name);
        }

        ms_[name] = member;

        uint16_t r = members_.size();
        members_.push_back(member);
        return r;
    }

    inline const std::vector<MemberInfo *> &get_members() const { return members_; }
    inline const MemberInfo &get_member(const int seq) const { return *(members_[seq]); }
    inline const MemberInfo &operator[](const int seq) const { return *(members_[seq]); }

    inline const MemberInfo &get_member(const std::string &name) const {
        auto it = ms_.find(name);
        if (it == ms_.end())
            throw std::runtime_error(std::string("no such member named [") + name + "] in struct[" +
                                     name_ + "]");

        return *(it->second);
    }

    inline const ParamListPtr &get_params(const size_t objaddr) const {
        return *((ParamListPtr *)(objaddr + coffset_));
    }
    inline const std::string &get_hash(const size_t objaddr) const {
        return *((std::string *)(objaddr + hoffset_));
    }
    inline const std::set<uint16_t> &get_seqs(const size_t objaddr) const {
        return *((std::set<uint16_t> *)(objaddr + soffset_));
    }

    inline void set_param(const size_t objaddr, const ParamListPtr &r) {
        *((ParamListPtr *)(objaddr + coffset_)) = r;
    }
    inline void set_hash(const size_t objaddr, const std::string &value) {
        ((*(std::string *)(objaddr + hoffset_))) = value;
    }
    inline void copy_seqs(const size_t objaddr, const std::set<uint16_t> &r) {
        auto &l = *((std::set<uint16_t> *)(objaddr + soffset_));
        for (auto v : r) l.insert(v);
    }

private:
    std::string name_;
    std::string alias_;

    size_t coffset_ = 0;
    size_t hoffset_ = 0;
    size_t soffset_ = 0;

    std::vector<MemberInfo *> members_;
    // just for searching by member name
    std::unordered_map<std::string, MemberInfo *> ms_;
};

class YanMember {
public:
    YanMember(const YanType type, const size_t voffset, const size_t foffset)
        : type_(type), voffset_(voffset), foffset_(foffset) {}

    inline const YanType yan_type() const { return type_; }

    inline const bool get_flag(const size_t objaddr) const {
        return (*((bool *)(objaddr + foffset_)));
    }
    template <typename T>
    inline const T &get_value(const size_t objaddr) const {
        return *((T *)(objaddr + voffset_));
    }

    inline void set_flag(const size_t objaddr) { (*((bool *)(objaddr + foffset_))) = true; }

    template <typename T>
    inline void set_value(const size_t objaddr, const T &t) {
        *((T *)(objaddr + voffset_)) = t;
        set_flag(objaddr);
    }
    template <typename T>
    inline void set_value(const size_t objaddr, T &&t) {
        *((T *)(objaddr + voffset_)) = std::forward<T>(t);
        set_flag(objaddr);
    }

private:
    YanType type_;

    size_t foffset_;
    size_t voffset_;
};

/***********************************field bridge****************************************/
inline std::string field_bridge(const YanStruct *a, const std::string &af, const YanStruct *b,
                                const std::string &bf) {
    assert(a && b);
    return a->get_name() + "." + a->get_member(af).get_name() + " = " + b->get_name() + "." +
           b->get_member(bf).get_name();
}

/***********************************register****************************************/
class __yan_member_register__ {
public:
    __yan_member_register__(YanStruct *obj, MemberInfo *info, const size_t voffset,
                            const size_t foffset, const std::function<void(uint16_t)> &cb) {
        info->member_ =
            new YanMember(info->type_, info->offset_ + voffset, info->offset_ + foffset);
        if (cb) cb(obj->add(info));
    }
};

class __yan_tmp_member_register__ {
public:
    __yan_tmp_member_register__(YanStruct *obj, YanStruct *from, const std::string &name,
                                const size_t offset, const size_t voffset, const size_t foffset) {
        assert(obj && from);

        bool found = false;
        auto &members = from->get_members();
        for (auto &member : members) {
            assert(member);
            if (member->name_ == name) {
                auto info =
                    new MemberInfo(from, member->name_, offset, member->type_, member->alias_);
                info->member_ =
                    new YanMember(info->type_, info->offset_ + voffset, info->offset_ + foffset);
                obj->add(info);
                found = true;
            }
        }

        if (!found)
            throw std::runtime_error(std::string("no such member [") + name + ("] in struct [") +
                                     from->get_origin_name() + "]");
    }
};

}  // namespace yan
