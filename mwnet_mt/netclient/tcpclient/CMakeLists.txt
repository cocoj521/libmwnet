set(tcpclient_SRCS
	TcpClient.cc
	Version.cc
   )

if(NOT CMAKE_BUILD_DYNAMIC_LIB)
	add_library(mwnet_mt_tcpclient ${tcpclient_SRCS})
else()
	add_library(mwnet_mt_tcpclient SHARED ${tcpclient_SRCS})
endif()

target_link_libraries(mwnet_mt_tcpclient mwnet_mt_netbase mwnet_mt_base)

install(TARGETS mwnet_mt_tcpclient DESTINATION lib)

set(HEADERS
	TcpClient.h
  )
install(FILES ${HEADERS} DESTINATION include/mwnet_mt/netclient)

add_executable(tcpclient_test tests/tcpclient_test.cpp)
target_link_libraries(tcpclient_test mwnet_mt_tcpclient mwnet_mt_util mwnet_mt_netbase mwnet_mt_base) 