cmake_minimum_required(VERSION 3.0)

project (mpack)


# compile in release  mode by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

option(BUILD_SHARED_LIBS "Build shared library" OFF)

if(UNIX)
  if(APPLE)
    set(PLAT "macosx")
    set(LINK_FLAGS "-bundle -undefined dynamic_lookup -all_load")
  else(APPLE)
    set(PLAT "linux")
    if(BUILD_SHARED_LIBS)
      set(LINK_FLAGS "-shared")
    endif()
  endif(APPLE)
  add_definitions(-D_GNU_SOURCE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb -Wall -fPIC -march=native -Wno-unused-parameter")
else(UNIX)
  if(WIN32)
    set(PLAT "win32")
    add_definitions(-DLUA_BUILD_AS_DLL -DLUA_LIB _D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /TP")
  else(WIN32)
    set(PLAT "unsupported")
  endif(WIN32)
endif(UNIX)

if (NOT LUA_INCLUDE_DIR OR NOT (BUILD_SHARED_LIBS OR LUA_LIBRARIES))
    message("Using system lua...")
    set(ENV{LUA_DIR} ~/.luaenv/versions/5.1.5) # hints for find lua in luaenv.
    find_package(Lua REQUIRED)
endif()

include_directories(${LUA_INCLUDE_DIR})

list(APPEND SOURCES
  src/mpack/mpack-common.c
  src/mpack/mpack-expect.c
  src/mpack/mpack-node.c
  src/mpack/mpack-platform.c
  src/mpack/mpack-reader.c
  src/mpack/mpack-writer.c
  src/luabind/lua_mpack.cpp
)
include_directories(${LUA_INCLUDE_DIR} src)

if(BUILD_SHARED_LIBS)
  add_library(mpack MODULE ${SOURCES})
  target_link_libraries(mpack ${LUA_LIBRARIES} ${LIBUV_LIBRARIES} ${OPENSSL_LIBRARIES})
  set_target_properties(mpack PROPERTIES OUTPUT_NAME "mpack" PREFIX "")
else()
  add_library(mpack ${SOURCES})
  set_target_properties(mpack PROPERTIES OUTPUT_NAME "luampack")
endif()

target_compile_features(mpack PRIVATE cxx_auto_type cxx_range_for cxx_nullptr)

if (LINK_FLAGS)
  set_target_properties(mpack PROPERTIES LINK_FLAGS ${LINK_FLAGS})
endif()

if (CMAKE_INSTALL_PREFIX)
  install(TARGETS mpack DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
endif()
