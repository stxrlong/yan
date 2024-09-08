
#pragma once

#include <gtest/gtest.h>

#include "sqlite/sqlite.h"
#include "sqlite_struct.h"

namespace yan {
namespace sqlite {

using Storage = Sqlite;

class TestSqlite : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        ASSERT_EQ(storage.init("./test_sqlite_db"), 0);
        // MaintainDB obj;
        // ComponentDB obj1;
        auto fut = storage.create_table<MaintainDB, ComponentDB, VersionDB>();
        try {
            ASSERT_EQ(fut.get(), 3);
        } catch (const std::exception& e) {
            logger_error("create table failed: %s", e.what());
            ASSERT_TRUE(false);
        }
    }

    static void TearDownTestSuite() {}

protected:
    static Storage storage;
};

}  // namespace sqlite
}  // namespace yan
