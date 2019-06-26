set(netserver_SRCS
	NetServer.cc
	UserOnlineList.cc
  )

if(NOT CMAKE_BUILD_DYNAMIC_LIB)
	add_library(mwnet_mt_netserver ${netserver_SRCS})
else()
	add_library(mwnet_mt_netserver SHARED ${netserver_SRCS})
endif()

target_link_libraries(mwnet_mt_netserver mwnet_mt_netbase mwnet_mt_base)

install(TARGETS mwnet_mt_netserver DESTINATION lib)

set(HEADERS
	NetServer.h
  )
install(FILES ${HEADERS} DESTINATION include/mwnet_mt/netserver)

add_executable(tcpclientlinux_test tests/tcpclientlinux.cpp)
target_link_libraries(tcpclientlinux_test pthread)

#add_executable(httpsrv_test tests/httpserver_test.cpp)
#target_link_libraries(httpsrv_test pthread mwnet_mt_netserver mwnet_mt_util)
       
add_executable(netserver_test tests/netserver_test.cpp)
target_link_libraries(netserver_test mwnet_mt_netserver mwnet_mt_util)    

add_executable(cmppserver_test tests/cmppserver_test.cpp)
target_link_libraries(cmppserver_test mwnet_mt_netserver mwnet_mt_util)  

#add_executable(netserver_test2 tests/netserver_test2.cpp)
#target_link_libraries(netserver_test2 mwnet_mt_netserver mwnet_mt_util)
