

#pragma once

#include "com/logger.h"
#include "ref.h"

namespace yan {

/*************************for sqlite/mysql***********************************/
template <typename RetObj>
inline const std::string& get_obj_name() {
    auto ys = RetObj::get_yan_struct();
    assert(ys);
    return ys->get_name();
}

/**
 * @brief this is only for those db that has prepared statement
 */
template <typename CondObj>
inline int copy_refobj(CondObj& lco, const CondObj& rco) {
    auto ys = CondObj::get_yan_struct();
    assert(ys);
    auto laddr = (size_t)(&lco);
    auto raddr = (size_t)(&rco);

    // 1. copy hash
    ys->set_hash(laddr, ys->get_hash(raddr));

    // 2. copy parameter
    auto& params = ys->get_params(raddr);
    if (params.size() > 0) ys->set_param(laddr, params);

    // 3. copy value
    auto& seqs = ys->get_seqs(raddr);
    if (seqs.size() > 0) ys->copy_seqs(laddr, seqs);

    return 0;
}

template <typename CondObj>
inline std::string get_obj_hash(const CondObj& co) {}

/**************************************read cond***********************************/
const std::string COUNT_FIELD = "-";
using RefObjFieldCallback = std::function<int(const std::string&, const std::string&)>;
class ReadCond {
public:
    ReadCond() = default;
    virtual ~ReadCond() {};

    template <typename CondObj, typename... Args>
    inline void append(CondObj& cond, Args&&... args) {
        cys_ = CondObj::get_yan_struct();

        auto addr = (size_t)&cond;
        params_ = cys_->get_params(addr);
        if (!params_)
            throw std::runtime_error("did you forget set parameter throw functions like AND/OR...");

        hash_ = cys_->get_hash(addr);

        return append(std::forward<Args>(args)...);
    }

    template <typename CondObj>
    inline void append() {
        cys_ = CondObj::get_yan_struct();
    }

    inline void append() { return; }

    virtual const bool ret_equal_to_cond() const = 0;
    virtual const int get_read_members(const RefObjFieldCallback& cb) const = 0;
    virtual const std::string hash_value() const = 0;
    virtual const std::string& get_obj_name() const = 0;

    inline const bool has_cond() const { return cys_ && params_; }

    // interal use
    /**
     * @attention please check the above func [has_obj_cond]
     * is true before calling the following funcs
     */
    inline const YanStruct* get_cond_struct() const { return cys_; }
    inline const ParamListPtr& get_params() const { return params_; }

protected:
    template <typename RetObj>
    inline const bool _ret_equal_to_cond() const {
        using Type = typename std::decay<RetObj>::type;
        return (!cys_ || (Type::get_yan_struct() == cys_));
    }

    template <>
    inline const bool _ret_equal_to_cond<int64_t>() const {
        return false;
    }

    template <typename RetObj>
    inline const int _get_read_members(const RefObjFieldCallback& cb) const {
        using Type = typename std::decay<RetObj>::type;
        auto ys = Type::get_yan_struct();

        int ret = 0;
        auto& members = ys->get_members();
        if (members.size() == 0) Type t;

        for (auto& member : members) {
            assert(member);
            auto from = member->from_;
            if ((ret = cb(from ? from->get_name() : "", member->get_name())) < 0) return ret;
        }

        return ret;
    }

    template <>
    inline const int _get_read_members<int64_t>(const RefObjFieldCallback& cb) const {
        if (!cys_) throw std::runtime_error("please specify a table before doing count operation");

        auto& members = cys_->get_members();
        if (members.size() == 0)
            throw std::runtime_error(
                "please specify a real table, not the tmp table, before doing count operation");

        int ret = 0;
        for (auto& member : members) {
            assert(member);
            if (member->type_ == YanType::AUTO_INCREMENT) continue;

            ret = cb(COUNT_FIELD, member->get_name());
            break;
        }

        return ret;
    }

    template <typename RetObj>
    inline const std::string _hash_value() const {
        using Type = typename std::decay<RetObj>::type;
        auto rys = Type::get_yan_struct();
        return !cys_ ? rys->get_name() : (rys->get_name() + cys_->get_name() + hash_);
    }

    template <>
    inline const std::string _hash_value<int64_t>() const {
        return !cys_ ? "" : (cys_->get_name() + hash_);
    }

    template <typename RetObj>
    inline const std::string& _get_obj_name() const {
        if (cys_)
            return cys_->get_name();
        else {
            using Type = typename std::decay<RetObj>::type;
            auto rys = Type::get_yan_struct();
            return rys->get_name();
        }
    }

    template <>
    inline const std::string& _get_obj_name<int64_t>() const {
        if (!cys_)
            throw std::runtime_error("please specify the table before doing count operation");

        return cys_->get_name();
    }

protected:
    YanStruct* cys_ = nullptr;
    ParamListPtr params_ = nullptr;
    std::string hash_;
};

template <typename Ret>
class ReadCondImpl : public ReadCond {
    using Type = typename SequenceValueType<Ret>::type;

public:
    ReadCondImpl() = default;
    ~ReadCondImpl() = default;

    const bool ret_equal_to_cond() const { return _ret_equal_to_cond<Type>(); }
    const int get_read_members(const RefObjFieldCallback& cb) const {
        return _get_read_members<Type>(cb);
    }
    const std::string hash_value() const { return _hash_value<Type>(); }
    const std::string& get_obj_name() const { return _get_obj_name<Type>(); }
};

/**************************************update cond***********************************/
class UpdateCond : public ReadCond {
public:
    UpdateCond() = default;
    ~UpdateCond() = default;

    virtual const std::string hash_value() const = 0;
    virtual const std::string& get_obj_name() const = 0;

    const size_t get_addr() const { return laddr_; }
    virtual const std::set<uint16_t>& get_seqs() const = 0;
    virtual const std::vector<MemberInfo*> get_members() const = 0;

private:
    const bool ret_equal_to_cond() const { return false; }
    const int get_read_members(const RefObjFieldCallback& cb) const { return -1; }

protected:
    size_t laddr_ = 0;
};

template <typename RefObj>
class UpdateCondImpl : public UpdateCond {
    using Type = typename std::decay<typename SequenceValueType<RefObj>::type>::type;

public:
    UpdateCondImpl() = default;
    ~UpdateCondImpl() = default;

    inline int copy_refobj(const RefObj& rco) {
        cys_ = Type::get_yan_struct();
        assert(cys_);
        laddr_ = (size_t)(&lco_);
        auto raddr = (size_t)(&rco);

        // 1. copy hash
        hash_ = cys_->get_hash(raddr);

        // 2. copy parameter
        params_ = cys_->get_params(raddr);

        // 3. copy value
        auto& seqs = cys_->get_seqs(raddr);
        if (seqs.size() > 0) {
            cys_->copy_seqs(laddr_, seqs);
            auto& ys = (*cys_);
            for (auto seq : seqs) {
                auto& info = ys[seq];

                switch (info.type_) {
                    case YanType::STRUCT:
                    case YanType::LIST:
                        break;
#define T(t, f)                                              \
    case YanType::f: {                                       \
        auto mem = info.member_;                             \
        assert(mem);                                         \
        mem->set_value<t>(laddr_, mem->get_value<t>(raddr)); \
    } break;

                        FOR_EACH_BASIC_TYPE(T)
                        FOR_EACH_SPECIAL_TYPE(T)
#undef T
                    default: {
                        throw std::runtime_error(
                            "unknown member type when copying the update object");
                    }
                }
            }
        }

        return 0;
    }

    const std::string hash_value() const {
        auto hash = _hash_value<Type>() /* cond hash */;

        // value hash
        auto& seqs = cys_->get_seqs((size_t)&lco_);
        for (auto seq : seqs) hash += std::to_string(seq);

        return hash;
    }
    const std::string& get_obj_name() const { return _get_obj_name<Type>(); }

    const std::set<uint16_t>& get_seqs() const { return cys_->get_seqs(laddr_); }
    const std::vector<MemberInfo*> get_members() const { return cys_->get_members(); }

private:
    Type lco_;  // reserve the value
};

/**************************************join read cond***********************************/
class JoinReadCond {
public:
    JoinReadCond() = default;
    ~JoinReadCond() = default;

    template <typename CondObj, typename... Args>
    inline void append(Args&&... args) {
        cys_ = CondObj::get_yan_struct();
        return append(std::forward<Args>(args)...);
    }

    // interal use
    inline const std::string& get_obj_name() const { return cys_->get_name(); }
    inline const std::list<std::string>& get_equals() const { return equals_; }

    inline std::string hash_value() const {
        std::string hash(cys_->get_name());
        for (auto& equal : equals_) hash += equal;

        return hash;
    }

private:
    template <typename... Args>
    inline void append(std::string&& equal, Args&&... args) {
        equals_.emplace_back(std::move(equal));

        return append(std::forward<Args>(args)...);
    }

    inline void append() {}

private:
    YanStruct* cys_ = nullptr;
    std::list<std::string> equals_;
};

/**************************************refops***********************************/
template <typename Obj, typename Concrete>
class RefOps {
public:
    RefOps() = default;
    ~RefOps() = default;

protected:
    template <typename RefObj>
    inline int assign_value(RefObj& ro, Obj& obj) {
        auto addr = (size_t)(&ro);

        auto ys = RefObj::get_yan_struct();
        assert(ys);
        auto& cys = *ys;
        auto& seqs = cys.get_seqs(addr);
        for (auto& seq : seqs) {
            auto& info = cys[seq];
            handle_info(obj, addr, info);
        }
        return 0;
    }

    template <typename RefObj>
    inline int set_value(RefObj& ro, Obj& obj) {
        auto addr = (size_t)(&ro);

        auto ys = RefObj::get_yan_struct();
        assert(ys);
        auto members = ys->get_members();
        for (auto& member : members) {
            handle_info(obj, addr, *member);
        }
        return 0;
    }

    inline int handle_info(Obj& obj, const size_t offset, const MemberInfo& info) {
        int ret = 0;
        switch (info.type_) {
            case YanType::STRUCT: {
                if ((ret = static_cast<Concrete&>(*this).struct_ref(obj, offset + info.offset_,
                                                                    info)) < 0)
                    return ret;
            } break;
            case YanType::LIST: {
                if ((ret = static_cast<Concrete&>(*this).list_ref(obj, offset, info)) < 0)
                    return ret;
            } break;
            default: {
                if ((ret = static_cast<Concrete&>(*this).member_ref(obj, offset, info)) < 0)
                    return ret;
            } break;
        }

        return ret;
    }
};

template <typename Obj>
using RefOpsCallback = std::function<int(Obj&)>;

/**************************************read ops***********************************/
template <typename T>
using ReadOpsCallback = std::function<const T&()>;

template <typename Obj, typename MemRefOps, typename ListRefOps, typename StructRefOps>
class ReadRefOps : public RefOps<Obj, ReadRefOps<Obj, MemRefOps, ListRefOps, StructRefOps>> {
    using Base = RefOps<Obj, ReadRefOps<Obj, MemRefOps, ListRefOps, StructRefOps>>;

public:
    ReadRefOps() = default;
    ~ReadRefOps() = default;

    template <typename RefObj>
    inline int append(Obj& obj, const RefObj& ro) {
        return Base::assign_value(ro, obj);
    }

    inline int append_value(Obj& obj, const UpdateCond& uo) {
        auto& infos = uo.get_members();
        auto& seqs = uo.get_seqs();

        if (seqs.size() == 0)
            throw std::runtime_error(
                "please set the values you want to update when doing update operation");

        auto addr = uo.get_addr();
        for (auto& seq : seqs) {
            auto& info = infos[seq];
            this->handle_info(obj, addr, *info);
        }

        return 0;
    }

    inline int append_cond(Obj& obj, const ReadCond& cond) {
        // we sure they are not null
        auto cys = *(cond.get_cond_struct());
        auto params = *(cond.get_params());

        int ret = 0;
        for (auto& param : params) {
            switch (param->yan_type()) {
                case YanType::STRUCT:
                case YanType::LIST:
                    break;
                default: {
                    if ((ret = cond_ref(obj, cys, *param)) < 0) return ret;
                }
            }
        }

        return ret;
    }

    template <typename RefObj>
    inline int append_field(Obj& obj) {
        auto ys = RefObj::get_yan_struct();
        assert(ys);

        int ret = 0;
        MemRefOps ops(obj);
        auto& members = ys->get_members();
        for (auto& member : members) {
            assert(member);
            switch (member->type_) {
                case YanType::STRUCT:
                case YanType::LIST:
                    break;
                default: {
                    if ((ret = ops.append(member->get_name(), member->type_, member->can_be_null_,
                                          member->length_)) < 0)
                        return ret;
                }
            }
        }

        return 0;
    }

protected:
    friend Base;

    inline int cond_ref(Obj& obj, const YanStruct& ys, const ParamBase& param) {
        auto& info = ys[param.yan_seq()];

        auto condtype = static_cast<CondType>(__GET_MEM_COND_TYPE__(param.yan_cond()));
        auto opstype = static_cast<YanOpsType>(__GET_MEM_OPS_TYPE__(param.yan_cond()));

        MemRefOps ops(obj, info.get_name());
        switch (param.yan_type()) {
#define T(t, r)                                                                   \
    case YanType::r: {                                                            \
        return ops.template append<t>(condtype, opstype, [&param]() -> const t& { \
            auto& pm = static_cast<const Param<t>&>(param);                       \
            return pm.get();                                                      \
        });                                                                       \
    }

            FOR_EACH_BASIC_TYPE(T)
            FOR_EACH_SPECIAL_TYPE(T)
#undef T

            default:
                assert(0);
        }

        return 0;
    }

    inline int member_ref(Obj& obj, const size_t offset, const MemberInfo& info) {
        auto& member = info.member_;
        assert(member);

        if (!member->get_flag(offset)) return 0;

        MemRefOps ops(obj, info.get_name());
        switch (member->yan_type()) {
#define T(t, r)                                                                                  \
    case YanType::r: {                                                                           \
        return ops.template append<t>(                                                           \
            CondType::AND, YanOpsType::EQUAL,                                                    \
            [&member, &offset]() -> const t& { return member->template get_value<t>(offset); }); \
    }

            FOR_EACH_BASIC_TYPE(T)
            FOR_EACH_SPECIAL_TYPE(T)
#undef T

            default:
                assert(0);
        }

        return 0;
    }

    inline int list_ref(Obj& obj, const size_t offset, const MemberInfo& info) {
        //         auto& listmember = info.list_member;
        //         assert(listmember);

        //         if (listmember->get_list_size(offset) == 0) return 0;

        //         ListRefOps ops(obj, listmember->get_name());
        //         switch (listmember->get_yan_type()) {
        //             case YanType::STRUCT:
        //                 return listmember->get_obj_value(offset, [this, &listmember, &ops](const
        //                 size_t offset) {
        //                     return ops.append([this, &listmember, offset](Obj& obj) {
        //                         auto sr = listmember->get_yan_struct();
        //                         assert(sr);

        //                         auto& members = sr->get_members();
        //                         return this->traverse(obj, offset, members);
        //                     });
        //                 });
        // #define T(t, r)                                 \
//     case YanType::r:                            \
//         return ops.template append<YanList<t>>( \
//             listmember->get_yan_type(),         \
//             [&offset, &listmember]() -> const YanList<t>& { return
        //             listmember->get_value<t>(offset); });
        //                 FOR_EACH_BASIC_TYPE(T)
        //                 FOR_EACH_SPECIAL_TYPE(T)
        // #undef T
        //             default:
        //                 assert(0);
        //         }

        return 0;
    }

    inline int struct_ref(Obj& obj, const size_t offset, const MemberInfo& info) {
        // auto& structmember = info.struct_member;
        // assert(structmember);

        // // we must use info's name here
        // StructRefOps ops(obj, (info.alias.empty() ? info.name : info.alias));
        // return ops.append([this, &structmember, &offset](Obj& obj) {
        //     auto& members = structmember->get_members();
        //     return this->traverse(obj, offset, members);
        // });
        return 0;
    }
};

/**************************************write refops***********************************/
template <typename T>
using WriteRefCallback = std::function<int(T&&)>;

template <typename Obj, typename MemRefOps, typename ListRefOps, typename StructRefOps>
class WriteRefOps : public RefOps<Obj, WriteRefOps<Obj, MemRefOps, ListRefOps, StructRefOps>> {
    using Base = RefOps<Obj, WriteRefOps<Obj, MemRefOps, ListRefOps, StructRefOps>>;

public:
    WriteRefOps() = default;
    ~WriteRefOps() = default;

    template <typename RefObj>
    inline int get(Obj& obj, RefObj& co) {
        return Base::set_value(co, obj);
    }

protected:
    friend Base;

    inline int member_ref(Obj& obj, const size_t offset, const MemberInfo& info) {
        auto& member = info.member_;
        assert(member);

        MemRefOps ops(obj, info.get_name());
        switch (member->yan_type()) {
#define T(t, r)                                                  \
    case YanType::r: {                                           \
        return ops.template get<t>([&member, &offset](t&& val) { \
            member->set_value<t>(offset, std::forward<t>(val));  \
            return 0;                                            \
        });                                                      \
    }
            FOR_EACH_BASIC_TYPE(T)
            FOR_EACH_SPECIAL_TYPE(T)
#undef T

            default:
                assert(0);
        }

        return 0;
    }

    inline int list_ref(Obj& obj, const size_t offset, const MemberInfo& info) {
        //         auto& listmember = info.list_member;
        //         assert(listmember);

        //         ListRefOps ops(obj, listmember->get_name());
        //         switch (listmember->get_yan_type()) {
        //             case YanType::STRUCT:
        //                 return listmember->set_obj_value(offset, [this, &listmember, &ops](const
        //                 size_t offset) {
        //                     return ops.get([this, &listmember, offset](Obj& obj) {
        //                         auto sr = listmember->get_yan_struct();
        //                         assert(sr);

        //                         auto& members = sr->get_members();
        //                         return this->traverse(obj, offset, members);
        //                     });
        //                 });
        // #define T(t, r) \
//     case YanType::r: \
//         return ops.template get(listmember->get_yan_type(), [&offset, &listmember](t&&
        //         val) { \
//             listmember->set_value<t>(offset, std::forward<t>(val)); \
//             return 0; \
//         });
        //                 FOR_EACH_BASIC_TYPE(T)
        //                 FOR_EACH_SPECIAL_TYPE(T)
        // #undef T

        //             default:
        //                 assert(0);
        //         }

        return 0;
    }

    inline int struct_ref(Obj& obj, const size_t offset, const MemberInfo& info) {
        // auto& structmember = info.struct_member;
        // assert(structmember);

        // StructRefOps ops(obj, structmember->get_name());
        // return ops.get([this, &structmember, &offset](Obj& obj) {
        //     auto& members = structmember->get_members();
        //     return this->traverse(obj, offset, members);
        // });
        return 0;
    }
};

}  // namespace yan
