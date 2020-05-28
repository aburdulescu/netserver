cmake_minimum_required(VERSION 3.10)

project(epolls)

option(EPOLLS_USE_ASAN "Use address sanitizer" ON)

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
  set(EPOLLS_USE_ASAN "OFF")
endif()

message(STATUS "EPOLLS_USE_ASAN: " ${EPOLLS_USE_ASAN})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "-Wall -Wextra")

if(EPOLLS_USE_ASAN)
  set(CMAKE_C_FLAGS "-Wall -Wextra -fno-omit-frame-pointer -fsanitize=address")
  set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
endif()

add_executable(epolls
  main.c
  net.c
  intvector.c
  server.c
  util.c
  )
target_link_libraries(epolls PUBLIC pthread rt)
