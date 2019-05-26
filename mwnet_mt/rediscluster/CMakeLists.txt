set_source_files_properties(RedisClient.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
set_source_files_properties(RedisClusterNode.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
set_source_files_properties(RedisClusterMgr.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")

set(rediscluster_SRCS
  RedisClient.cpp
  RedisClusterNode.cpp
  RedisClusterMgr.cpp
)

add_library(rediscluster ${rediscluster_SRCS})

#install(TARGETS rediscluster DESTINATION lib)
#file(GLOB HEADERS "*.h")
#install(FILES ${HEADERS} DESTINATION include/rediscluster)

