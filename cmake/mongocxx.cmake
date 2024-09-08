
if(NOT Boost_FOUND)
	message(STATUS "find Boost")

	link_directories(/usr/local/lib)
	include_directories(/usr/local/include/bsoncxx/v_noabi/)
	include_directories(/usr/local/include/mongocxx/v_noabi/)
endif()

list(APPEND DEPS_LIBS mongocxx bsoncxx)