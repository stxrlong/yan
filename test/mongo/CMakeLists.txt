set(test_program test_mongo)

file(GLOB_RECURSE TEST_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(${test_program} ${TEST_SRCS})
target_link_libraries(${test_program} PUBLIC ${DEPS_LIBS})
