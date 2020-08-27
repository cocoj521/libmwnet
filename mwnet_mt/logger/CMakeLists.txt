set(logger_SRCS
	../base/Exception.cc
	../base/Thread.cc
  ../base/Condition.cc
  ../base/CountDownLatch.cc
	../base/Timestamp.cc
	../base/TimeZone.cc
	../base/LogStream.cc
	../base/FileUtil.cc
	../base/AsyncLogging.cc
	../base/Logging.cc
	../base/LogFile.cc
	../base/AsyncLogging.cc
	./MWLogger.cc
	Version.cc
  )

if(NOT CMAKE_BUILD_DYNAMIC_LIB)
	add_library(mwlogger ${logger_SRCS})
else()
	add_library(mwlogger SHARED ${logger_SRCS})
endif()

target_link_libraries(mwlogger pthread rt)

install(TARGETS mwlogger DESTINATION lib)

set(HEADERS
	../base/Exception.h
	../base/Thread.h
  ../base/Condition.h
  ../base/CountDownLatch.h
	../base/Timestamp.h
	../base/CurrentThread.h
	../base/TimeZone.h
	../base/LogStream.h
	../base/StringPiece.h
	../base/Types.h
	../base/FileUtil.cc
	../base/AsyncLogging.h
	../base/Logging.cc
	../base/LogFile.h
	../base/AsyncLogging.h
	./MWLogger.h
  )
#file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/mwnet_mt/logger)

add_executable(logger_mwlogger_test tests/logger_test.cpp) 
target_link_libraries(logger_mwlogger_test mwlogger)