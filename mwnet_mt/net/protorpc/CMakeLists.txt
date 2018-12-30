add_custom_command(OUTPUT rpc.pb.cc rpc.pb.h
  COMMAND protoc
  ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS rpc.proto
  VERBATIM )

set_source_files_properties(rpc.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion")
include_directories(${PROJECT_BINARY_DIR})

add_library(mwnet_mt_protorpc_wire rpc.pb.cc RpcCodec.cc)
set_target_properties(mwnet_mt_protorpc_wire PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")

if(NOT CMAKE_BUILD_NO_EXAMPLES)
add_executable(protobuf_rpc_wire_test RpcCodec_test.cc)
target_link_libraries(protobuf_rpc_wire_test mwnet_mt_protorpc_wire mwnet_mt_protobuf_codec)
set_target_properties(protobuf_rpc_wire_test PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
endif()

add_library(mwnet_mt_protorpc RpcChannel.cc RpcServer.cc)
set_target_properties(mwnet_mt_protorpc PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(mwnet_mt_protorpc mwnet_mt_protorpc_wire mwnet_mt_protobuf_codec mwnet_mt_netbase protobuf z)

if(TCMALLOC_LIBRARY)
  target_link_libraries(mwnet_mt_protorpc tcmalloc_and_profiler)
endif()

install(TARGETS mwnet_mt_protorpc_wire mwnet_mt_protorpc DESTINATION lib)

set(HEADERS
  RpcCodec.h
  RpcChannel.h
  RpcServer.h
  rpc.proto
  rpcservice.proto
  ${PROJECT_BINARY_DIR}/mwnet_mt/net/protorpc/rpc.pb.h
  )
install(FILES ${HEADERS} DESTINATION include/mwnet_mt/net/protorpc)

