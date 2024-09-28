

#include "test_mongo.h"

namespace yan {
namespace mongo {

Storage TestMongoDB::storage;

TEST_F(TestMongoDB, mongo_write) {
    Data data;
    data.name = "MongoDB";

    logger_debug("test write one entity");
    auto wfut = storage.write(data);
    try {
        ASSERT_EQ(wfut.get(), 1);
    } catch (const std::exception& e) {
        logger_error("write one entity failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test write one entity ok");

    std::list<Data> datas;
    datas.emplace_back(std::move(data));
    logger_debug("test write many entities");
    auto wfut1 = storage.write(datas);
    try {
        ASSERT_EQ(wfut1.get(), 1);
    } catch (const std::exception& e) {
        logger_error("write many entities failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test write many entities ok");
}

TEST_F(TestMongoDB, mongo_read) {
    Data cond;
    cond.name.AND() == "mongodb";
    cond.versions.AND() == std::initializer_list<std::string>{"v6.0", "v5.0"};

    logger_debug("test read one entity");
    auto rfut = storage.read<std::list<Data>>(cond);
    try {
        ASSERT_EQ(rfut.get().size(), 1);
    } catch (const std::exception& e) {
        logger_error("read one entity failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test read one entity ok");
}

TEST_F(TestMongoDB, mongo_delete) {
    Data cond;
    cond.name.AND() == "mongodb";

    logger_debug("test delete one entity");
    auto rfut = storage.del(cond);
    try {
        ASSERT_EQ(rfut.get(), 1);
    } catch (const std::exception& e) {
        logger_error("delete one entity failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("delete read one entity ok");
}

}  // namespace mongo
}  // namespace yan
