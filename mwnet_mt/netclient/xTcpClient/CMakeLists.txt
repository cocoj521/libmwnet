set(tcpclient_SRCS
	TcpClient.cc
   )

if(NOT CMAKE_BUILD_DYNAMIC_LIB)
	add_library(mwnet_mt_xtcpclient ${tcpclient_SRCS})
else()
	add_library(mwnet_mt_xtcpclient SHARED ${tcpclient_SRCS})
endif()

target_link_libraries(mwnet_mt_xtcpclient mwnet_mt_netbase mwnet_mt_base)

install(TARGETS mwnet_mt_xtcpclient DESTINATION lib)

set(HEADERS
	TcpClient.h
  )
install(FILES ${HEADERS} DESTINATION include/mwnet_mt/netclient)

add_executable(xtcpclient_test tests/tcpclient_test.cpp)
target_link_libraries(xtcpclient_test mwnet_mt_xtcpclient mwnet_mt_util mwnet_mt_netbase mwnet_mt_base) 