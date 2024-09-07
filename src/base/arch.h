

#pragma once

#include "ref.h"

namespace yan {

/*********************************structure operator*********************************/
/**
 * @brief define a yan struct named obj
 * @param obj struct name
 */
#define STRUCT(obj, args...) __YAN_STRUCT__(obj, ##args)

/**
 * @brief define a member for yan struct obj
 * @param obj struct name
 * @param mem member name
 * @param type member type, only support c/c++ basic type
 */
#define MEMBER(obj, mem, type, args...) __YAN_MEMBER__(obj, mem, type, ##args)

/**
 * @brief define a list member with basic data type for yan struct obj
 * @param obj struct name
 * @param mem list member name
 * @param type member type, only support c/c++ basic type
 */
// #define LIST_MEMBER(obj, mem, type, args...) __YAN_LIST_MEMBER__(obj, mem, type, ##args)

/**
 * @brief define an yan struct list member that the yan struct object is
 * defined by yourself for the top struct obj
 * @param obj top struct name
 * @param mem list member name
 * @param type member type, only support yan struct type define by yourself
 */
// #define LIST_STRUCT_MEMBER(obj, mem, type, args...) __YAN_LIST_STRUCT_MEMBER__(obj, mem,
// type,##args)

/**
 * @brief define an yan struct object member for the top struct obj
 * @param obj top struct name
 * @param mem struct member name
 * @param type member type, only support yan struct type define by yourself
 */
// #define STRUCT_MEMBER(obj, mem, type, args...) __YAN_STRUCT_MEMBER__(obj, mem, type, ##args)

/**
 * @brief end of the struct
 */
#define STRUCT_END __YAN_STRUCT_END__

/********************************tmp structure operator********************************/
/**
 * @brief define a yan temporary struct named obj, this temporary struct will
 * not be reflected to the real db
 * @param obj struct name
 */
#define TMP_STRUCT(obj) __YAN_TMP_STRUCT__(obj)

/**
 * @brief define a temporary member for yan tmp struct obj
 * @param obj struct name
 * @param mem member name
 * @param from the member that is from the real db struct you define by STRUCT and MEMBER
 */
#define TMP_MEMBER(obj, mem, from) __YAN_TMP_MEMBER__(obj, mem, from)

/**
 * @brief end of the temporary struct
 */
#define TMP_STRUCT_END __YAN_STRUCT_END__

/**
 * @brief for sql
 */
#define CAN_BE_NULL false

/*********************************JOIN_READ_CONDITION*********************************/
#define FIELD_BRIDGE(OBJ1, FIELD1, OBJ2, FIELD2) __YAN_FIELD_BRIDGE__(OBJ1, FIELD1, OBJ2, FIELD2)

}  // namespace yan