
#pragma once

#include <stdio.h>

namespace yan {

#define logger_trace(fmt, args...) printf("TRACE [%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##args)
#define logger_debug(fmt, args...) printf("DEBUG [%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##args)
#define logger_info(fmt, args...) printf("INFO [%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##args)
#define logger_warn(fmt, args...) printf("WARN [%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##args)
#define logger_error(fmt, args...) printf("ERROR [%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##args)
#define logger_fatal(fmt, args...) printf("FATAL [%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##args)

}