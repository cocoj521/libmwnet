set(http_SRCS
  HttpServer.cc
  HttpResponse.cc
  HttpContext.cc
  )

if(CMAKE_BUILD_DYNAMIC_LIB)
	add_library(mwnet_mt_http SHARED ${http_SRCS})
else()
	add_library(mwnet_mt_http ${http_SRCS})
endif()

target_link_libraries(mwnet_mt_http mwnet_mt_netbase)

install(TARGETS mwnet_mt_http DESTINATION lib)
set(HEADERS
  HttpRequest.h
  HttpResponse.h
  HttpServer.h
  )
install(FILES ${HEADERS} DESTINATION include/mwnet_mt/net/http)

if(NOT CMAKE_BUILD_NO_EXAMPLES)
  add_executable(httpserver_test tests/HttpServer_test.cc)
target_link_libraries(httpserver_test mwnet_mt_http)

if(BOOSTTEST_LIBRARY)
  add_executable(httprequest_unittest tests/HttpRequest_unittest.cc)
  target_link_libraries(httprequest_unittest mwnet_mt_http boost_unit_test_framework)
endif()

endif()
