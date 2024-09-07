find_package(ConcurrentQueue CONFIG)
if(NOT ConcurrentQueue_FOUND)
	message(STATUS "find ConcurrentQueue")

	set(CONCURRENTQUEUE_DIRECTORY ${CONCURRENTQUEUE_PATH}/)
	message(STATUS "CONCURRENTQUEUE_DIRECTORY:${CONCURRENTQUEUE_DIRECTORY}")

	include_directories(${CONCURRENTQUEUE_DIRECTORY}/)
endif()
