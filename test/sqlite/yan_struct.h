

#pragma once

#include "base/arch.h"

namespace yan {
namespace sqlite {

STRUCT(MaintainDB, tm)
MEMBER(MaintainDB, id, YanType::AUTO_INCREMENT)
MEMBER(MaintainDB, name, YanType::STRING, n)
MEMBER(MaintainDB, type, YanType::INT32, t, CAN_BE_NULL)
MEMBER(MaintainDB, point, YanType::INT32, p)
MEMBER(MaintainDB, data, YanType::INT32, d)
MEMBER(MaintainDB, online, YanType::DATE, o)
STRUCT_END

STRUCT(ComponentDB, tc)
MEMBER(ComponentDB, id, YanType::AUTO_INCREMENT)
MEMBER(ComponentDB, component, YanType::STRING, c)
MEMBER(ComponentDB, name, YanType::STRING, n)
MEMBER(ComponentDB, version, YanType::STRING, v)
MEMBER(ComponentDB, his_version, YanType::STRING, h)
STRUCT_END

STRUCT(VersionDB, tv)
MEMBER(VersionDB, id, YanType::AUTO_INCREMENT)
MEMBER(VersionDB, name, YanType::STRING, n)
MEMBER(VersionDB, data, YanType::INT32, d)
STRUCT_END

enum UsageType {
    UNKNOWN_USAGE_TYPE = 0,
    NO_SQL,
    STRUCTURED,
    K_V,
};

enum PointType {
    UNKNOWN_POINT_TYPE = 0,
    CLIENT,
    SERVER,
    BOTH,
};

enum DataType {
    UNKNOWN_DATA_TYPE = 0,
    MANAGEMENT,
    TEMPORARY,
    CACHE,
    HISTORY,
};

}  // namespace sqlite
}  // namespace yan
