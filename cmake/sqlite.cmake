find_package(Sqlite CONFIG)
if(NOT Sqlite_FOUND)
	message(STATUS "find Sqlite")

	set(SQLITE_DIRECTORY ${SQLITE_PATH}/)
	message(STATUS "SQLITE_DIRECTORY:${SQLITE_DIRECTORY}")

	include_directories(${SQLITE_DIRECTORY}/include/)
	link_directories(${SQLITE_DIRECTORY}/lib/)
endif()

list(APPEND DEPS_LIBS sqlite3)
