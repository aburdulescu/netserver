cmake_minimum_required(VERSION 3.10)

project(evserver)

if(CMAKE_BUILD_TYPE EQUAL Debug)
  set(CMAKE_C_FLAGS "-Wall -Werror")
endif()

add_compile_definitions(EV_STANDALONE)
add_executable(evserver
  main.c
  libev/ev.c
  ../cserver/net.c
  )
target_include_directories(evserver PUBLIC libev ../cserver)
