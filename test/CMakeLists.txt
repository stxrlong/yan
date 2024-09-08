list(APPEND DEPS_LIBS ${PROJECT_NAME} gtest)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/test)

message(STATUS "TEST-DEPS_LIBS:${DEPS_LIBS}")

add_subdirectory(sqlite)
add_subdirectory(mongo)