// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <string>
#include <vector>

struct lua_State;

namespace hg {

class LuaObject;

/// Push a value from a VM to a different VM.
int PushForeign(lua_State *L, const LuaObject &value);

/// Set a value in a different VM.
void SetForeign(lua_State *L, const std::string &key, const LuaObject &value);
void SetForeign(const LuaObject &table, const std::string &key, const LuaObject &value);
void SetForeign(const LuaObject &table, int key, const LuaObject &value);

/// Call a function with arguments possibly coming from a different VM.
bool CallForeign(const LuaObject &function, const std::vector<LuaObject> &args, std::vector<LuaObject> *ret_vals);
bool CallForeign(lua_State *L, const char *name, const std::vector<LuaObject> &args, std::vector<LuaObject> *ret_vals);

} // namespace hg
