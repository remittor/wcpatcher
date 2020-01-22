cmake_minimum_required(VERSION 3.9)
set(TRJ wcpatcher)
project(${TRJ})

set(CMAKE_CXX_STANDARD 14)

include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/src/win32common
  ${CMAKE_CURRENT_SOURCE_DIR}/extlibs/asmjit/src
  ${CMAKE_CURRENT_SOURCE_DIR}/extlibs/xxHash
)

add_compile_options( -fms-extensions )
add_compile_options( -fpermissive )
add_compile_options( -Wmultichar )

add_library(${TRJ} OBJECT
  src/bst/srw_lock.cpp
  src/winntapi.cpp
  src/log.cpp
  src/utils.cpp
  src/wcp_cfg.cpp
  src/wcp_filecache.cpp
  src/wcp_filetree.cpp
  src/wcp_hook.cpp
  src/wcp_memory.cpp
  src/wcp_plugin.cpp
  src/main.cpp
)
