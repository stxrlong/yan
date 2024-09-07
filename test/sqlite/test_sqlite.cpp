

#include "test_sqlite.h"

#include "yan_struct.h"

namespace yan {
namespace sqlite {

Storage TestSqlite::storage;

TEST_F(TestSqlite, write) {
    /***************************write only one entity**************************/
    MaintainDB obj;
    obj.name = "MySQL";
    obj.type = UsageType::STRUCTURED;
    obj.point = PointType::SERVER;
    obj.data = DataType::MANAGEMENT;
    obj.online = 20220102;

    logger_debug("test write one entity");
    // normal sql: INSERT INTO table (field, field1, ...) VALUES (value, value1, ...)
    auto wfut = storage.write(obj);
    try {
        ASSERT_EQ(wfut.get(), 1);
    } catch (const std::exception& e) {
        logger_error("write one entity failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test write one entity ok");

    /*****************write many entities by sequence container****************/
    MaintainDB obj1;
    obj1.name = "MongoDB";
    obj1.type = UsageType::NO_SQL;
    obj1.point = PointType::SERVER;
    obj1.data = DataType::HISTORY;
    obj1.online = 20220605;

    MaintainDB obj2;
    obj2.name = "REDIS";
    obj2.type = UsageType::K_V;
    obj2.point = PointType::SERVER;
    obj2.data = DataType::CACHE;
    obj2.online = 20220515;

    MaintainDB obj3;
    obj3.name = "SQLITE";
    obj3.type = UsageType::STRUCTURED;
    obj3.point = PointType::CLIENT;
    obj3.data = DataType::TEMPORARY;
    obj3.online = 20220312;

    std::list<MaintainDB> objs;
    objs.emplace_back(obj1);
    objs.emplace_back(obj2);
    objs.emplace_back(obj3);

    logger_debug("test write many entities via sequence container");
    // INSERT INTO table (field, field1, ...) VALUES (value, value1, ...), (value, value1, ...), ...
    auto wfut1 = storage.write(objs);
    try {
        ASSERT_EQ(wfut1.get(), 3);
    } catch (const std::exception& e) {
        logger_error("write many entities by sequence container failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test write many entities through sequence container ok");

    /**************************write many entities one by on*************************/
    ComponentDB cnpnt;
    cnpnt.component = "UserMngr";
    cnpnt.name = "MySQL";
    cnpnt.version = "v5.5";
    cnpnt.his_version = "v5.0";

    ComponentDB cnpnt1;
    cnpnt1.component = "SpaceMngr";
    cnpnt1.name = "MongoDB";
    cnpnt1.version = "v3.10.1";
    cnpnt1.his_version = "v3.9.0";

    ComponentDB cnpnt2;
    cnpnt2.component = "NetMngr";
    cnpnt2.name = "Redis";
    cnpnt2.version = "v6.0";
    cnpnt2.his_version = "v5.6";

    ComponentDB cnpnt3;
    cnpnt3.component = "FileMngr";
    cnpnt3.name = "Sqlite";
    cnpnt3.version = "v3.6.0";
    cnpnt3.his_version = "v3.5.0";

    logger_debug("test write many entities one by one");
    // INSERT INTO table (field, field1, ...) VALUES (value, value1, ...);
    // INSERT INTO table (field, field1, ...) VALUES (value, value1, ...);
    auto wfut2 = storage.write(cnpnt, cnpnt1, cnpnt2, cnpnt3);
    try {
        ASSERT_EQ(wfut2.get(), 4);
    } catch (const std::exception& e) {
        logger_error("write many entities one by one failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test write many entities one by one ok");
}

TEST_F(TestSqlite, read) {
    /***************************read only one entity**************************/
    MaintainDB obj;
    obj.name.AND() == "MySQL";

    logger_debug("test read only one entity");
    // [SELECT * FROM table WHERE field = value] which returns onle one entity;
    auto rfut = storage.read(obj);
    try {
        auto ret = rfut.get();
        EXPECT_EQ(ret.name.VALUE(), "MySQL");
        EXPECT_EQ(ret.type.VALUE(), UsageType::STRUCTURED);
        EXPECT_EQ(ret.point.VALUE(), PointType::SERVER);
        EXPECT_EQ(ret.data.VALUE(), DataType::MANAGEMENT);
        EXPECT_EQ(ret.online.VALUE(), 20220102);
    } catch (const std::exception& e) {
        logger_error("read only one entity failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test read only one entity ok");

    /***************************read many entities with no condition**************************/
    logger_debug("test read list/vector with no condition");
    // SELECT * FROM table
    auto rfut2 = storage.read<std::list<MaintainDB>>();
    try {
        auto ret = rfut2.get();
        EXPECT_EQ(ret.size(), 4);
    } catch (const std::exception& e) {
        logger_error("read list/vector with no condition failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test read list/vector with no condition ok");

    /***************************read many entities with condition**************************/
    logger_debug("test read list/vector with condition");
    // [SELECT * FROM table WHERE field = value] which returns many entities
    auto rfut3 = storage.read<std::vector<MaintainDB>>(obj);
    try {
        auto ret = rfut3.get();
        EXPECT_EQ(ret.size(), 1);
    } catch (const std::exception& e) {
        logger_error("read list/vector with condition failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test read list/vector with condition ok");
}

TEST_F(TestSqlite, update) {
    /***************************update**************************/
    ComponentDB obj;
    obj.version = "v5.6";
    obj.name.AND() == "MySQL";

    logger_debug("test update");
    // UPDATE tabel SET field = value, field1 = value1 ... WHERE field = Cond ...
    auto ufut = storage.update(obj);
    try {
        auto ret = ufut.get();
        EXPECT_EQ(ret, 1);
    } catch (const std::exception& e) {
        logger_error("update failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test update ok");

    /***************************read only one entity**************************/
    logger_debug("test read only one entity");
    // [SELECT * FROM table WHERE field = value] which returns onle one entity;
    auto rfut = storage.read(obj);
    try {
        auto ret = rfut.get();
        EXPECT_EQ(ret.name.VALUE(), "MySQL");
        EXPECT_EQ(ret.version.VALUE(), "v5.6");
    } catch (const std::exception& e) {
        logger_error("read only one entity failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test read only one entity ok");
}

TEST_F(TestSqlite, count) {
    /***************************count with condition**************************/
    ComponentDB obj;
    obj.name.AND() == "MySQL";

    logger_debug("test count with condition");
    // SELECT COUNT(field) FROM table WHERE field = value ...
    auto cfut = storage.count(obj);
    try {
        auto ret = cfut.get();
        EXPECT_EQ(ret, 1);
    } catch (const std::exception& e) {
        logger_error("count with condition failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test count with condition ok");

    /***************************count without condition**************************/
    logger_debug("test count without condition");
    // SELECT COUNT(field) FROM table
    auto cfut1 = storage.count<ComponentDB>();
    try {
        auto ret = cfut1.get();
        EXPECT_EQ(ret, 4);
    } catch (const std::exception& e) {
        logger_error("count without condition failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test count without condition ok");
}

TEST_F(TestSqlite, del) {
    /***************************delete with condition**************************/
    ComponentDB obj;
    obj.name.AND() == "Sqlite";

    logger_debug("test delete with condition");
    // DELETE FROM table WHERE field = value ...
    auto dfut = storage.del(obj);
    try {
        auto ret = dfut.get();
        // EXPECT_EQ(ret, 1);
    } catch (const std::exception& e) {
        logger_error("delete with condition failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test delete with condition ok");
}

}  // namespace sqlite
}  // namespace yan
