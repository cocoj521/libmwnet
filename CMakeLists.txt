cmake_minimum_required(VERSION 2.6)

project(mwnet_mt C CXX)

enable_testing()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CXX_FLAGS
 -g
 # -DVALGRIND
 -DMWNET_MT_STD_STRING
 #-DCHECK_PTHREAD_RETURN_VALUE
 -D_FILE_OFFSET_BITS=64
 -Wall
 -Wextra
 -Werror
 -Wconversion
 -Wno-unused-parameter
 -Wold-style-cast
 -Woverloaded-virtual
 -Wpointer-arith
 -Wshadow
 -Wwrite-strings
 #-march=native
 #-MMD
 -std=c++11
 -fPIC
 -rdynamic
 )
if(CMAKE_BUILD_BITS EQUAL 32)
  list(APPEND CXX_FLAGS "-m32")
endif()
if(CMAKE_BUILD_DYNAMIC_LIB)
  list(APPEND CXX_FLAGS "-shared")
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

find_package(Boost REQUIRED)
find_package(CURL)
find_package(ZLIB)
find_path(CARES_INCLUDE_DIR ares.h)
find_library(CARES_LIBRARY NAMES cares)
find_library(BOOSTPO_LIBRARY NAMES boost_program_options)
find_path(TCMALLOC_INCLUDE_DIR gperftools/heap-profiler.h)
find_library(TCMALLOC_LIBRARY NAMES tcmalloc_and_profiler)

if(CARES_INCLUDE_DIR AND CARES_LIBRARY)
  message(STATUS "found cares")
endif()
if(CURL_FOUND)
  message(STATUS "found curl")
endif()
if(TCMALLOC_INCLUDE_DIR AND TCMALLOC_LIBRARY)
  message(STATUS "found tcmalloc")
endif()
if(ZLIB_FOUND)
  message(STATUS "found zlib")
endif()


include_directories(${Boost_INCLUDE_DIRS})

include_directories(${PROJECT_SOURCE_DIR})

include_directories(${PROJECT_SOURCE_DIR}/mwnet_mt/include)
link_directories(${PROJECT_SOURCE_DIR}/mwnet_mt/lib)

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

add_subdirectory(mwnet_mt/base)
add_subdirectory(mwnet_mt/crypto)
add_subdirectory(mwnet_mt/net)
add_subdirectory(mwnet_mt/netserver)
add_subdirectory(mwnet_mt/netclient)
add_subdirectory(mwnet_mt/netclient/curl)
add_subdirectory(mwnet_mt/logger) 
add_subdirectory(mwnet_mt/util)