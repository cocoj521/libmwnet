include(CheckFunctionExists)

check_function_exists(accept4 HAVE_ACCEPT4)
if(NOT HAVE_ACCEPT4)
  set_source_files_properties(SocketsOps.cc PROPERTIES COMPILE_FLAGS "-DNO_ACCEPT4")
endif()

set(net_SRCS
  Acceptor.cc
  Buffer.cc
  Channel.cc
  Connector.cc
  EventLoop.cc
  EventLoopThread.cc
  EventLoopThreadPool.cc
  InetAddress.cc
  Poller.cc
  poller/DefaultPoller.cc
  poller/EPollPoller.cc
  poller/PollPoller.cc
  httpparser/httpparser.cpp
  httpparser/http_parser.c
  Socket.cc
  SocketsOps.cc
  TcpClient.cc
  TcpConnection.cc
  TcpServer.cc
  Timer.cc
  TimerQueue.cc
  Version.cc
  )

if(NOT CMAKE_BUILD_DYNAMIC_LIB)
	add_library(mwnet_mt_netbase ${net_SRCS})
else()
	add_library(mwnet_mt_netbase SHARED ${net_SRCS})
endif()

target_link_libraries(mwnet_mt_netbase mwnet_mt_base)

install(TARGETS mwnet_mt_netbase DESTINATION lib)

set(HEADERS
  Buffer.h
  Callbacks.h
  Channel.h
  Endian.h
  EventLoop.h
  EventLoopThread.h
  EventLoopThreadPool.h
  InetAddress.h
  TcpClient.h
  TcpConnection.h
  TcpServer.h
  TimerId.h
  )
install(FILES ${HEADERS} DESTINATION include/mwnet_mt/net)

#add_subdirectory(http)
#add_subdirectory(httpclient)
#add_subdirectory(inspect)
#add_subdirectory(httpclient)
#add_subdirectory(libmwnet_mt)

if(NOT CMAKE_BUILD_NO_EXAMPLES)
  #add_subdirectory(tests)
endif()

#if(PROTOBUF_FOUND)
#  add_subdirectory(protobuf)
#  add_subdirectory(protorpc)
#else()
#  add_subdirectory(protobuf EXCLUDE_FROM_ALL)
#  add_subdirectory(protorpc EXCLUDE_FROM_ALL)
#endif()
