// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "fabgen.h"

#include "engine/lua_object.h"
#include "engine/physics.h"
#include "engine/scene.h"

#include "bind_Lua.h"

#include "foundation/profiler.h"
#include "foundation/quaternion.h"
#include "foundation/rect.h"

#include "script/lua_vm.h"

extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

namespace hg {

int PushForeign(lua_State *L, const LuaObject &o) {
	if (o.L() == L) {
		o.Push(); // fast path
		return 1;
	} else {
		LuaStackGuard guard(o.L());
		o.Push();

		if (lua_isinteger(o.L(), -1) == 1) {
			const auto v = lua_tointeger(o.L(), -1);
			lua_pushinteger(L, v);
			return 1;
		} else if (lua_isnumber(o.L(), -1) == 1) {
			const auto v = lua_tonumber(o.L(), -1);
			lua_pushnumber(L, v);
			return 1;
		} else if (lua_isboolean(o.L(), -1) == 1) {
			const auto v = lua_toboolean(o.L(), -1);
			lua_pushboolean(L, v);
			return 1;
		} else if (lua_isstring(o.L(), -1) == 1) {
			const auto v = lua_tostring(o.L(), -1);
			lua_pushstring(L, v);
			return 1;
		} else if (const auto type_tag = hg_lua_get_wrapped_object_type_tag(o.L(), -1)) {
			if (const auto info = hg_lua_get_bound_type_info(type_tag)) {
				void *obj;
				info->to_c(o.L(), -1, &obj);
				__ASSERT__(obj != nullptr);
				return info->from_c(L, obj, Copy);
			}
		}
	}
	return 0;
}

void SetForeign(lua_State *L, const std::string &key, const LuaObject &val) {
	if (PushForeign(L, val))
		lua_setglobal(L, key.c_str());
}

void SetForeign(const LuaObject &table, const std::string &key, const LuaObject &val) {
	table.Push();

	if (PushForeign(table.L(), val))
		lua_setfield(table.L(), -2, key.c_str());
	else
		lua_pop(table.L(), 1);
}

void SetForeign(const LuaObject &table, int key, const LuaObject &val) {
	table.Push();

	if (PushForeign(table.L(), val))
		lua_seti(table.L(), -2, key);
	else
		lua_pop(table.L(), 1);
}

//
bool CallForeign(const LuaObject &function, const std::vector<LuaObject> &args, std::vector<LuaObject> *ret_vals) {
	ProfilerPerfSection slice("LuaVM.CallForeign");

	const auto L = function.L(); // call will happen on this VM

	LuaStackGuard stack_guard(L);
	PushCustomErrorHandler(L);

	const auto stack_top = lua_gettop(L);

	function.Push();
	for (auto &arg : args)
		PushForeign(L, arg);

	ResetExecutionWatchdog(L);
	if (lua_pcall(L, numeric_cast<int>(args.size()), ret_vals != nullptr ? LUA_MULTRET : 0, -int(2 + args.size())) != LUA_OK) // perform call
		return false;

	if (ret_vals)
		GatherReturnValues(L, stack_top, *ret_vals);

	return true;
}

bool CallForeign(lua_State *L, const char *name, const std::vector<LuaObject> &args, std::vector<LuaObject> *ret_vals) {
	return CallForeign(Get(L, name), args, ret_vals);
}

} // namespace hg
