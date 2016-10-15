#pragma once
#ifndef __LUA_MPACK_HPP__
#define __LUA_MPACK_HPP__

#if __cplusplus
extern "C" {
#endif

#include "lua.h"
#include "lauxlib.h"

int luaopen_mpack(lua_State* L);

#if __cplusplus
}
#endif

#endif
