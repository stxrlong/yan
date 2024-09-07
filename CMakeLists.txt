cmake_minimum_required(VERSION 3.16.3)

project(yan)

option(BUILD_EXAMPLES "build example" OFF)
option(BUILD_TESTS "build test" ON)

set(CMAKE_CXX_STANDARD 11)
add_compile_options(-fPIC)
add_definitions(
	-DBOOST_THREAD_PROVIDES_FUTURE 
	-DBOOST_THREAD_PROVIDES_FUTURE_CONTINUATION 
	-DBOOST_ERROR_CODE_HEADER_ONLY
)
add_definitions(
	-DBOOST_THREAD_PROVIDES_EXECUTORS
	-DBOOST_THREAD_USES_MOVE
)

if(${BUILD_TESTS} OR ${BUILD_EXAMPLES})
	add_compile_options("-g")
	set(CMAKE_BUILD_TYPE Debug)
else()
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(boost)
include(sqlite)
include(concurrentqueue)

add_subdirectory(src)

if(${BUILD_TEST})
	include(CTest)
	enable_testing()
	add_subdirectory(test)
endif()