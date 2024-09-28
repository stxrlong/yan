
#pragma once

#include <gtest/gtest.h>

#include "mongo/mongo.h"
#include "mongo_struct.h"

namespace yan {
namespace mongo {

using Storage = MongoDB;

class TestMongoDB : public ::testing::Test {
protected:
    static void SetUpTestSuite() { ASSERT_EQ(storage.init(""), 0); }

    static void TearDownTestSuite() {}

protected:
    static Storage storage;
};

}  // namespace mongo
}  // namespace yan
