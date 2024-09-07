
if(NOT Boost_FOUND)
	message(STATUS "find Boost")

	link_directories(/usr/local/lib)
	include_directories(/usr/local/include/)
endif()

list(APPEND DEPS_LIBS boost_system boost_program_options boost_thread)