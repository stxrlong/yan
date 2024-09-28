

#include <gtest/gtest.h>

#include "mongo/mongo_writeref.h"
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

TEST(TestMongoWriteRef, mongofetch) {
    document doc;
    doc << "name" << "MongoDB" << "versions" << open_array << "v6.0"
        << "v5.0" << close_array << "drivers" << open_array << open_document << "lang" << "cxx"
        << close_document << close_array << "info" << open_document << "x" << "203"
        << close_document;

    logger_debug("value bson: %s", bsoncxx::to_json(doc.view()).c_str());

    Data data;

    FetchMongoResult ops;
    int ret = ops.fetch(data, doc.view());
    ASSERT_EQ(ret, 0);

    logger_debug("data.name: %s", data.name.VALUE().c_str());

    std::string vs;
    auto count = 0;
    for (auto &v : data.versions) {
        if (++count != 1) vs += ", ";
        vs += v;
    }

    logger_debug("data.versions: %s", vs.c_str());

    std::string ds;
    count = 0;
    ds += "[ ";
    for (auto &d : data.drivers) {
        if (++count != 1) ds += ", ";
        ds += "{ lang: ";
        ds += d.lang;
        ds += " }";
    }
    ds += " ]";

    logger_debug("data.drivers: %s", ds.c_str());
    logger_debug("data.info.x: %s", data.info.x.VALUE().c_str());
}

}  // namespace mongo
}  // namespace yan
