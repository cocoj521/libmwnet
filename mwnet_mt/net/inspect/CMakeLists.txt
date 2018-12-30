set(inspect_SRCS
  Inspector.cc
  PerformanceInspector.cc
  ProcessInspector.cc
  SystemInspector.cc
  )

add_library(mwnet_mt_inspect ${inspect_SRCS})
target_link_libraries(mwnet_mt_inspect mwnet_mt_http)

if(TCMALLOC_INCLUDE_DIR AND TCMALLOC_LIBRARY)
  set_target_properties(mwnet_mt_inspect PROPERTIES COMPILE_FLAGS "-DHAVE_TCMALLOC")
  target_link_libraries(mwnet_mt_inspect tcmalloc_and_profiler)
endif()

install(TARGETS mwnet_mt_inspect DESTINATION lib)
set(HEADERS
  Inspector.h
  )
install(FILES ${HEADERS} DESTINATION include/mwnet_mt/net/inspect)

if(NOT CMAKE_BUILD_NO_EXAMPLES)
add_executable(inspector_test tests/Inspector_test.cc)
target_link_libraries(inspector_test mwnet_mt_inspect)
endif()

