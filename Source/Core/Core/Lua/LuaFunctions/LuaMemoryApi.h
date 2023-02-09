#pragma once
#include <string>
#include <lua.hpp>
#include "Common/CommonTypes.h"
#include "Core/Lua/LuaHelperClasses/LuaColonCheck.h"

namespace Lua::LuaMemoryApi
{

void InitLuaMemoryApi(lua_State* lua_state, const std::string& lua_api_version);

int DoGeneralRead(lua_State* lua_state);
int DoReadUnsignedBytes(lua_State* lua_state);
int DoReadSignedBytes(lua_State* lua_state);
int DoReadFixedLengthString(lua_State* lua_state);
int DoReadNullTerminatedString(lua_State* lua_state);

int DoGeneralWrite(lua_State* lua_state);
int DoWriteBytes(lua_State* lua_state);
int DoWriteString(lua_State* lua_state);
}  // namespace LuaMemoryApi
