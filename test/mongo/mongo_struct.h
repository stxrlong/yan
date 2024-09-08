

#pragma once

#include "base/arch.h"

namespace yan {
namespace mongo {

// The following structure comes from the mongocxx driver's example:
// {
//     "name" : "MongoDB",
//   "type" : "database",
//   "count" : 1,
//   "versions": [ "v6.0", "v5.0", "v4.4", "v4.2", "v4.0", "v3.6" ],
//   "drivers" : [{
//     "lang" : "cxx",
//     "addr" : "https://xxx",
//     "downloads" : 123
//   }, {
//     "lang" : "java",
//     "addr" : "https://xxx1",
//     "downloads" : 120
//   }],
//   "info" : {
//         "x" : "203", "y" : 102
//     }
// }

// drivers
STRUCT(Driver)
MEMBER(Driver, lang, YanType::STRING)
MEMBER(Driver, addr, YanType::STRING)
MEMBER(Driver, download, YanType::INT32)
STRUCT_END

// info
STRUCT(Info)
MEMBER(Info, x, YanType::STRING)
MEMBER(Info, y, YanType::INT32)
STRUCT_END

// data
STRUCT(Data)
MEMBER(Data, name, YanType::STRING)
MEMBER(Data, type, YanType::STRING)
MEMBER(Data, count, YanType::INT32)
/* different from the normal member */
LIST_MEMBER(Data, versions, YanType::STRING)
LIST_STRUCT_MEMBER(Data, drivers, Driver)
STRUCT_MEMBER(Data, info, Info)
STRUCT_END

}  // namespace mongo
}  // namespace yan
