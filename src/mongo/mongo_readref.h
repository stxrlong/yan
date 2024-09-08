

#pragma once

#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/json.hpp>

#include "base/ref_ops.h"

namespace yan {
namespace mongo {
using bsoncxx::builder::stream::array_context;
using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::key_context;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
using bsoncxx::document::element;
using bsoncxx::document::view;

class MakeWriteBson {
    class ReadMemRef;
    class ReadListMem;
    class ReadStructMem;
    using MongoReadRefOps = ReadRefOps<document, ReadMemRef, ReadListMem, ReadStructMem>;

    MongoReadRefOps ops_;

public:
    MakeWriteBson() = default;
    ~MakeWriteBson() = default;

    template <typename ObjRef>
    document make(const ObjRef& obj) {
        document doc;
        int ret = ops_.append(doc, obj);
        if (ret < 0)
            throw std::runtime_error(std::string("mongo makes write bson obj failed:") +
                                     std::to_string(ret));

        return doc;
    }

private:
    /**************************Read Member Ref***********************/
    class ReadMemRef {
    public:
        ReadMemRef(document& doc, const std::string& name) : doc_(doc), name_(name) {}
        ~ReadMemRef() = default;

        template <typename T>
        inline int append(const CondType condtype, const YanOpsType opstype,
                          const std::function<const T&()>& cb) {
            doc_ << name_ << cb();
            return 0;
        }

    private:
        const std::string& name_;
        document& doc_;
    };

    /**************************Read List Member***********************/
    class ReadListMem {
    public:
        ReadListMem(document& doc, const std::string& name)
            : doc_(doc), array_(doc << name << open_array) {}
        ~ReadListMem() { array_ << close_array; }

        inline int append(const RefOpsCallback<document>& cb) {
            document doc;
            int ret = cb(doc);
            if (ret < 0) return ret;

            array_ << std::move(doc);
            return 0;
        }

        template <typename T>
        inline int append(const YanList<T>& values) {
            for (auto& t : values) array_ << t;
            return 0;
        }

    private:
        document& doc_;
        array_context<key_context<>> array_;
    };

    /**************************Read Struct Mem***********************/
    class ReadStructMem {
    public:
        ReadStructMem(document& doc, const std::string& name) : doc_(doc), name_(name) {}
        ~ReadStructMem() = default;

        inline int append(const RefOpsCallback<document>& cb) {
            document doc;
            int ret = cb(doc);
            if (ret < 0) return ret;

            doc_ << name_ << std::move(doc);
            return 0;
        }

    private:
        const std::string& name_;
        document& doc_;
    };
};

class MakeCondBson {
    class ReadMemRef;
    using MongoReadRefOps = ComReadRef<document, ReadMemRef>::ComReadRefOps;
    MongoReadRefOps ops_;

public:
    MakeCondBson() = default;

    template <typename ObjRef>
    document make(const ObjRef& obj) {
        document doc;
        int ret = ops_.append_refobj_cond(doc, obj);
        if (ret < 0)
            throw std::runtime_error(std::string("mongo makes condition bson failed:") +
                                     std::to_string(ret));

        return doc;
    }

private:
    /**************************Read Member Ref***********************/
    class ReadMemRef {
    public:
        ReadMemRef(document& doc, const std::string& name) : doc_(doc), name_(name) {}
        ~ReadMemRef() = default;

        template <typename T>
        inline int append(const CondType condtype, const YanOpsType opstype,
                          const std::function<const T&()>& cb) {
            return append_value(cb);
        }

    private:
        template <typename T>
        inline int append_value(const std::function<const std::list<T>&()>& cb) {
            auto array = doc_ << name_ << open_document << "$eleMatch" << open_array;
            auto& l = cb();
            for (auto& v : l) array << v;
            array << close_array;
            doc_ << close_document;
            return 0;
        }

        template <typename T>
        inline int append_value(const std::function<const T&()>& cb) {
            doc_ << name_ << cb();
            return 0;
        }

    private:
        const std::string& name_;
        document& doc_;
    };
};

}  // namespace mongo
}  // namespace yan
