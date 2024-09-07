

#include "test_sqlite.h"

namespace yan {
namespace sqlite {

TMP_STRUCT(JoinObj)
TMP_MEMBER(JoinObj, id, MaintainDB);
TMP_MEMBER(JoinObj, name, MaintainDB);
TMP_MEMBER(JoinObj, version, ComponentDB);
TMP_MEMBER(JoinObj, his_version, ComponentDB);
TMP_STRUCT_END

TEST_F(TestSqlite, join_read) {
    logger_debug("test join read one entity");
    auto rfut = storage.join_read<MaintainDB>()
                    .left_join<ComponentDB>(FIELD_BRIDGE(MaintainDB, name, ComponentDB, name))
                    .commit<JoinObj>();
    try {
        auto ret = rfut.get();
        EXPECT_EQ(ret.id.VALUE(), 0);
        EXPECT_EQ(ret.name.VALUE(), "MySQL");
        EXPECT_EQ(ret.version.VALUE(), "v5.6");
    } catch (const std::exception& e) {
        logger_error("join read one entity failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test join read one entity ok");

    logger_debug("test join read one entity with condition");
    MaintainDB cond;
    cond.type.AND() == UsageType::NO_SQL;
    auto rfut1 = storage.join_read<MaintainDB>()
                     .left_join<ComponentDB>(FIELD_BRIDGE(MaintainDB, name, ComponentDB, name))
                     .commit<JoinObj>(cond);
    try {
        auto ret = rfut1.get();
        EXPECT_EQ(ret.id.VALUE(), 0);
        EXPECT_EQ(ret.name.VALUE(), "MongoDB");
        EXPECT_EQ(ret.version.VALUE(), "v3.10.1");
    } catch (const std::exception& e) {
        logger_error("join read one entity with condition failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test join read one entity with condition ok");
}

TEST_F(TestSqlite, copy) {
    logger_debug("test join copy with condition");
    MaintainDB cond;
    cond.type.AND() == UsageType::NO_SQL;
    auto rfut = storage.copy<MaintainDB>().to<VersionDB>(cond);
    try {
        auto ret = rfut.get();
        EXPECT_EQ(ret, 0);
    } catch (const std::exception& e) {
        logger_error("join copy with condition failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test join copy with condition ok");
}

TMP_STRUCT(SubMaintainDB)
TMP_MEMBER(SubMaintainDB, id, MaintainDB);
TMP_MEMBER(SubMaintainDB, name, MaintainDB);
TMP_STRUCT_END

TMP_STRUCT(SubComponentDB)
TMP_MEMBER(SubComponentDB, id, ComponentDB);
TMP_MEMBER(SubComponentDB, name, ComponentDB);
TMP_STRUCT_END

TEST_F(TestSqlite, union_read) {
    logger_debug("test union read one entity");
    MaintainDB obj;
    obj.point.AND() == PointType::SERVER;
    ComponentDB obj1;
    obj1.version.AND() == "v5.6";
    auto rfut = storage.union_read<SubMaintainDB>(obj)
                    .union_read<SubComponentDB>(obj1)
                    .commit<std::list<SubMaintainDB>>();
    try {
        auto ret = rfut.get();
        if (ret.size() > 0) {
            for (auto& r : ret) {
                logger_debug("id: %lld, name: %s", r.id.VALUE(), r.name.VALUE().c_str());
            }
        }
    } catch (const std::exception& e) {
        logger_error("union read one entity failed: %s", e.what());
        ASSERT_TRUE(false);
    }
    logger_debug("test union read one entity ok");
}
}  // namespace sqlite
}  // namespace yan
