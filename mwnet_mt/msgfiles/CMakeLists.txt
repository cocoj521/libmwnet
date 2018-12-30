set(msgfiles_SRCS
	DirUtil.cpp
	Error.cpp
	FileMgr.cpp
	MsgFiles.cpp
	MsgFilesImpl.cpp
  )

if(NOT CMAKE_BUILD_DYNAMIC_LIB)
	add_library(msgfiles ${msgfiles_SRCS})
else()
	add_library(msgfiles SHARED ${msgfiles_SRCS})
endif()

target_link_libraries(msgfiles)

install(TARGETS msgfiles DESTINATION lib)

set(HEADERS
	DirUtil.h
	Error.h
	FileMgr.h
	MsgFiles.h
	MsgFilesImpl.h
  )
install(FILES ${HEADERS} DESTINATION include/mwnet_mt/msgfiles)

add_executable(test_msgfiles tests/test_msgfiles.cpp)
target_link_libraries(test_msgfiles msgfiles pthread)
