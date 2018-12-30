set(netclient_SRCS
	httpclient.cpp
	TcpClient.cpp
	NetClient.cpp
  )

if(NOT CMAKE_BUILD_DYNAMIC_LIB)
	add_library(mwnet_mt_netclient ${netclient_SRCS})
else()
	add_library(mwnet_mt_netclient SHARED ${netclient_SRCS})
endif()

target_link_libraries(mwnet_mt_netclient mwnet_mt_netbase mwnet_mt_base)

install(TARGETS mwnet_mt_netclient DESTINATION lib)

set(HEADERS
  httpclient.h
  TcpClient.h
  NetClient.h
  )
install(FILES ${HEADERS} DESTINATION include/mwnet_mt/netclient)

add_executable(httpclient_test tests/httpclient_test.cc)
add_executable(tcpclient_test tests/TcpClientTest.cpp)
add_executable(TcpClientRmsTest_test tests/TcpClientRmsTest.cpp)
target_link_libraries(tcpclient_test mwnet_mt_netclient)
target_link_libraries(httpclient_test mwnet_mt_netclient)
target_link_libraries(TcpClientRmsTest_test mwnet_mt_netclient)
