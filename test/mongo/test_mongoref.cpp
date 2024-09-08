

#include <gtest/gtest.h>

#include "mongo/mongo_readref.h"
#include "mongo_struct.h"

namespace yan {
namespace mongo {

TEST(TestMongoReadRef, mongowrite) {
    Data data;
    data.name = "MongoDB";
    data.type = "database";
    data.count = 1;
    data.versions = {"v6.0", "v5.0", "v4.4", "v4.2", "v4.0", "v3.6"};
    // data.versions.push_back("6.0");

    Driver driver;
    driver.lang = "cxx";
    driver.addr = "https://xxx";
    driver.download = 123;
    Driver driver1;
    driver1.lang = "java";
    driver1.addr = "https://xxx1";
    driver1.download = 120;

    data.drivers.emplace_back(driver);
    data.drivers.emplace_back(driver1);

    auto &info = data.info;
    info.x = "203";
    info.y = 102;

    MakeWriteBson ops;
    auto doc = ops.make(data);

    logger_debug("write bson: %s", bsoncxx::to_json(doc.view()).c_str());
}

TEST(TestMongoReadRef, mongoread) {
    Data data;
    data.name.AND() == "MongoDB";

    data.versions.AND() == std::initializer_list<std::string>{"v6.0", "v5.0"};

    data.drivers.COND().lang.AND() == "cxx";

    auto &info = data.info;
    info.x.AND() == "203";
    info.y.AND() == 102;

    MakeCondBson ops;
    auto doc = ops.make(data);

    logger_debug("cond bson: %s", bsoncxx::to_json(doc.view()).c_str());
}

}  // namespace mongo
}  // namespace yan
