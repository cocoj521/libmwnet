add_library(mwnet_mt_protobuf_codec ProtobufCodecLite.cc)
set_target_properties(mwnet_mt_protobuf_codec PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(mwnet_mt_protobuf_codec mwnet_mt_netbase protobuf z)

install(TARGETS mwnet_mt_protobuf_codec DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/mwnet_mt/net/protobuf)

