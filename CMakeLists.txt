cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(riffdb LANGUAGES C)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  add_compile_options(
    -Ofast
    -fno-stack-protector
    -ffast-math 
    -march=native 
    -mtune=native 
  )
  add_definitions(
    -DNDEBUG -DNTRACE
  )

  add_subdirectory(libs/mimalloc)
  link_libraries(
    mimalloc-static
  )
else()
  add_compile_options(-fsanitize=address,undefined -g3)
  add_link_options(-fsanitize=address,undefined -g3)

  add_definitions(
    -DSQLITE_DEBUG -DTRACE -DDEBUG
  )
endif()

include_directories(./sqlite)
include_directories(./yyjson)
include_directories(./src)

add_compile_options(
  -Wno-nan-infinity-disabled
  -Wall -Wextra -Wpedantic
  -Wformat=2 -Wno-unused-parameter -Wshadow
  -Wwrite-strings -Wstrict-prototypes -Wold-style-definition
  -Wredundant-decls -Wnested-externs -Wmissing-include-dirs
  -Wno-format-nonliteral
  -Wjump-misses-init -Wlogical-op
)

file(GLOB_RECURSE SOURCES src/*.c)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} 
  yyjson/yyjson.c 
  sqlite/sqlite3.c
)
