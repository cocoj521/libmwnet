#link_directories(${PROJECT_SOURCE_DIR}/mwnet_mt/lib/curl)
#link_directories(${PROJECT_SOURCE_DIR}/mwnet_mt/lib)

set(curl_SRCS
	CurlRequest.cpp
	CurlManager.cpp
	#CurlEvMgr.cpp
	RequestManager.cpp
	CurlHttpClient.cpp
	Version.cc
  )

if(NOT CMAKE_BUILD_DYNAMIC_LIB)
	add_library(mwnet_mt_curl ${curl_SRCS})
else()
	add_library(mwnet_mt_curl SHARED ${curl_SRCS})
endif()

target_link_libraries(mwnet_mt_curl mwnet_mt_netbase mwnet_mt_base mwnet_mt_util curl ssl crypto z cares pthread dl event_core event_pthreads)
#target_link_libraries(mwnet_mt_curl mwnet_mt_netbase mwnet_mt_base curl ssl crypto z)

install(TARGETS mwnet_mt_curl DESTINATION lib)

set(HEADERS
#	CurlRequest.h
#	CurlManager.h
#	CurlEvMgr.h
#	RequestManager.h
	CurlHttpClient.h
  )
install(FILES ${HEADERS} DESTINATION include/mwnet_mt/netclient)

add_executable(jhb_curl_test tests/curl_test.cpp)
target_link_libraries(jhb_curl_test mwnet_mt_curl mwnet_mt_base mwnet_mt_netbase pthread rt)
