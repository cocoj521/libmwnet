set(base_SRCS
  AsyncLogging.cc
  Condition.cc
  CountDownLatch.cc
  Date.cc
  Exception.cc
  FileUtil.cc
  LogFile.cc
  Logging.cc
  LogStream.cc
  ProcessInfo.cc
  Timestamp.cc
  TimeZone.cc
  Thread.cc
  ThreadPool.cc
  )

if(NOT CMAKE_BUILD_DYNAMIC_LIB)
	add_library(mwnet_mt_base ${base_SRCS})
else()
	add_library(mwnet_mt_base SHARED ${base_SRCS})
endif()

target_link_libraries(mwnet_mt_base pthread rt)

install(TARGETS mwnet_mt_base DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/mwnet_mt/base)

if(NOT CMAKE_BUILD_NO_EXAMPLES)
  add_subdirectory(tests)
endif()
