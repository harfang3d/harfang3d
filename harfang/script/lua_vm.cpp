// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "script/lua_vm.h"

#include "foundation/assert.h"
#include "foundation/cext.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/profiler.h"
#include "foundation/time.h"

extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

#ifndef LUA_OK
#define LUA_OK 0
#endif

namespace hg {

LuaObject::LuaObject() : L_(nullptr), ref(LUA_NOREF) {}
LuaObject::LuaObject(LuaObject &&o) : L_(o.L_), ref(o.ref) {
	o.L_ = nullptr;
	o.ref = LUA_NOREF;
}

LuaObject::LuaObject(const LuaObject &o) : L_(o.L_) {
	if (o.ref != LUA_NOREF) {
		lua_rawgeti(L_, LUA_REGISTRYINDEX, o.ref);
		ref = luaL_ref(L_, LUA_REGISTRYINDEX);
	}
}

LuaObject &LuaObject::operator=(const LuaObject &o) {
	Clear();
	if (o.ref != LUA_NOREF) {
		L_ = o.L();
		lua_rawgeti(L_, LUA_REGISTRYINDEX, o.ref);
		ref = luaL_ref(L_, LUA_REGISTRYINDEX);
	}
	return *this;
}

LuaObject &LuaObject::operator=(LuaObject &&o) {
	Clear();
	L_ = o.L_;
	ref = o.ref;
	o.L_ = nullptr;
	o.ref = LUA_NOREF;
	return *this;
}

LuaObject::operator bool() const { return (ref != LUA_NOREF) && (ref != LUA_REFNIL); }

void LuaObject::Push() const { lua_rawgeti(L_, LUA_REGISTRYINDEX, ref); }

void LuaObject::Clear() {
	// FIXME catch dangling VM pointers
	if (ref != LUA_NOREF) {
		luaL_unref(L_, LUA_REGISTRYINDEX, ref);
		ref = LUA_NOREF;
	}
}

LuaObject::~LuaObject() { Clear(); }

//
LuaStackGuard::LuaStackGuard(lua_State *L_) : L(L_), top(lua_gettop(L_)) {}
LuaStackGuard::~LuaStackGuard() { lua_settop(L, top); }

//
void SetVMName(lua_State *L, const std::string &name) {
	lua_pushstring(L, name.c_str());
	lua_setglobal(L, "__VM_name");
}

std::string GetVMName(lua_State *L) {
	std::string name("Unnamed VM");
	lua_getglobal(L, "__VM_name");
	if (lua_isstring(L, -1))
		name = lua_tostring(L, -1);
	return name;
}

//
static void _watchdog_hook(lua_State *L, lua_Debug *ar) {
	if (!PingExecutionWatchdog(L)) {
		lua_getinfo(L, "l", ar);
		luaL_error(L, "Execution stuck at line %d, script has been forcefully stopped.", ar->currentline);
	}
}

void ResetExecutionWatchdog(lua_State *L) {
	lua_pushinteger(L, time_now());
	lua_setglobal(L, "__VM_watchdog_timestamp");
}

void InstallExecutionWatchdog(lua_State *L, time_ns timeout) {
	SetExecutionWatchdogTimeout(L, timeout);
	lua_sethook(L, &_watchdog_hook, LUA_MASKCOUNT, 1000);
}

void SetExecutionWatchdogTimeout(lua_State *L, time_ns timeout) {
	lua_pushinteger(L, timeout);
	lua_setglobal(L, "__VM_watchdog_timeout");
}

bool PingExecutionWatchdog(lua_State *L) {
	lua_getglobal(L, "__VM_watchdog_timeout");
	lua_getglobal(L, "__VM_watchdog_timestamp");

	if (!lua_isinteger(L, -2) || !lua_isinteger(L, -1))
		return false;

	const time_ns timeout = lua_tointeger(L, -2);
	const time_ns timestamp = lua_tointeger(L, -1);
	lua_pop(L, 2);

	const time_ns t_now = time_now();
	const bool alive = (t_now - timestamp) < timeout;

	lua_pushinteger(L, t_now);
	lua_setglobal(L, "__VM_watchdog_timestamp");
	return alive;
}

//
LuaObject Pop(lua_State *L) { return LuaObject(L, luaL_ref(L, LUA_REGISTRYINDEX)); }

//
LuaObject MakeLuaObj(lua_State *L, bool v) {
	lua_pushboolean(L, v);
	return Pop(L);
}

LuaObject MakeLuaObj(lua_State *L, int v) {
	lua_pushinteger(L, v);
	return Pop(L);
}

LuaObject MakeLuaObj(lua_State *L, float v) {
	lua_pushnumber(L, v);
	return Pop(L);
}

LuaObject MakeLuaObj(lua_State *L, const std::string &v) {
	lua_pushstring(L, v.c_str());
	return Pop(L);
}

//
bool LuaObjValue(const LuaObject &o, bool v) {
	o.Push();
	if (lua_isboolean(o.L(), -1))
		v = lua_toboolean(o.L(), -1);
	lua_pop(o.L(), 1);
	return v;
}

int LuaObjValue(const LuaObject &o, int v) {
	o.Push();
	if (lua_isinteger(o.L(), -1))
		v = int(lua_tointeger(o.L(), -1));
	lua_pop(o.L(), 1);
	return v;
}

float LuaObjValue(const LuaObject &o, float v) {
	o.Push();
	if (lua_isnumber(o.L(), -1))
		v = float(lua_tonumber(o.L(), -1));
	lua_pop(o.L(), 1);
	return v;
}

std::string LuaObjValue(const LuaObject &o, std::string v) {
	o.Push();
	if (lua_isstring(o.L(), -1))
		v = lua_tostring(o.L(), -1);
	lua_pop(o.L(), 1);
	return v;
}

//
LuaObject Get(lua_State *L, const std::string &key) {
	lua_getglobal(L, key.c_str());
	return {L, luaL_ref(L, LUA_REGISTRYINDEX)};
}

LuaObject Get(const LuaObject &table, int index) {
	table.Push();
	lua_rawgeti(table.L(), -1, index);
	LuaObject o(table.L(), luaL_ref(table.L(), LUA_REGISTRYINDEX));
	lua_pop(table.L(), 1);
	return o;
}

LuaObject Get(const LuaObject &table, const std::string &key) {
	table.Push();
	lua_getfield(table.L(), -1, key.c_str());
	LuaObject o(table.L(), luaL_ref(table.L(), LUA_REGISTRYINDEX));
	lua_pop(table.L(), 1);
	return o;
}

//
void Set(const std::string &key, const LuaObject &value) {
	value.Push();
	lua_setglobal(value.L(), key.c_str());
}

void Set(const LuaObject &table, int key, const LuaObject &value) {
	__ASSERT__(table.L() == value.L());
	table.Push();
	value.Push();
	lua_rawseti(table.L(), -2, key);
	lua_pop(table.L(), 1);
}

void Set(const LuaObject &table, const std::string &key, const LuaObject &value) {
	__ASSERT__(table.L() == value.L());
	table.Push();
	value.Push();
	lua_setfield(table.L(), -2, key.c_str());
	lua_pop(table.L(), 1);
}

//
bool Compile(lua_State *L, const std::string &source, const std::string &name, LuaObject &chunk_out) {
	ProfilerPerfSection slice("LuaVM.Compile");
	if (luaL_loadbuffer(L, source.c_str(), source.length(), name.c_str()) != 0) {
		const auto err = lua_tostring(L, -1);
		warn(format("Lua VM error: %1").arg(err).c_str());
		return false;
	}
	chunk_out = Pop(L);
	return true;
}

//
void GetAndReportError(lua_State *L) {
	const auto err = lua_tostring(L, -1);
	warn(format("%1: %2").arg(GetVMName(L)).arg(err).c_str());
	lua_pop(L, 1);
}

//
static LuaCallstack CaptureCallstack(lua_State *L) {
	LuaCallstack callstack;

	lua_Debug ar;
	for (int depth = 0; lua_getstack(L, depth, &ar); ++depth) {
		const auto status = lua_getinfo(L, "Sln", &ar);
		if (!status)
			break;

		auto &frame = *callstack.frames.emplace(callstack.frames.end());
		frame.file = ar.source;
		frame.line = ar.currentline;

		std::string what(ar.what);

		if (what == "C")
			frame.location = format("%1 C function").arg(ar.name ? ar.name : "? (C/C++)").str();
		else
			frame.location = format("%1 Lua function in %2 at line %3").arg(ar.name ? ar.name : "?").arg(ar.source).arg(ar.currentline).str();

		// capture locals
		/*
				for (uint32_t i = 1;; ++i) {
					const auto name = lua_getlocal(L, &ar, i);
					if (!name)
						break;

					if (!strcmp(name, "(*temporary)"))
						continue; // skip temporaries

					frame.locals.emplace_back();
					auto &local_var = frame.locals.back();
					local_var.name = name;
					// GetTypeValueFromLua(L, local_var.value, -1);
					lua_pop(L, 1); // pop value from stack
				}
		*/
	}
	return callstack;
}

//
static int _error_handler(lua_State *L) {
	std::string msg = format("----\nRuntime error on Lua VM '%1':\n\t%2\n").arg(GetVMName(L)).arg(lua_tostring(L, -1)).str();
	lua_pop(L, 1); // pop error msg

	auto callstack = CaptureCallstack(L);
	msg += "Callstack:\n";

	uint32_t depth = 0;
	for (auto &frame : callstack.frames)
		msg += format("\t%1 - %2\n").arg(depth++).arg(frame.location).str();

	warn(msg.c_str()); // send full error to log
	lua_pushstring(L, msg.c_str()); // and back to Lua

	// if (vm->emit_runtime_error_signal)
	//	vm->runtime_error_signal.Emit(err);
	return 1;
}

void PushCustomErrorHandler(lua_State *L) { lua_pushcfunction(L, &_error_handler); }

//
void GatherReturnValues(lua_State *L, int start_index, std::vector<LuaObject> &ret_vals) {
	const auto count = lua_gettop(L) - start_index;
	ret_vals.resize(count);
	for (auto i = 0; i < count; ++i)
		ret_vals[count - i - 1] = Pop(L); // pop and grab a reference to the return value...
}

bool Execute(lua_State *L, const std::string &source, const std::string &name, LuaObject *env, std::vector<LuaObject> *ret_vals) {
	ProfilerPerfSection slice("LuaVM.Execute");

	LuaStackGuard stack_guard(L);
	PushCustomErrorHandler(L);

	const auto stack_top = lua_gettop(L);

	ResetExecutionWatchdog(L);
	if (luaL_loadbuffer(L, source.c_str(), source.length(), name.c_str()) != LUA_OK) {
		const auto err = lua_tostring(L, -1);
		warn((std::string("Lua VM error: ") + err).c_str());
		return false;
	}

	if (env) {
		env->Push();
		if (lua_setupvalue(L, -2, 1) == nullptr)
			return false;
	}

	if (lua_pcall(L, 0, LUA_MULTRET, -2) != LUA_OK)
		return false;

	if (ret_vals)
		GatherReturnValues(L, stack_top, *ret_vals);

	return true;
}

bool Call(const LuaObject &function, const std::vector<LuaObject> &args, std::vector<LuaObject> *ret_vals) {
	ProfilerPerfSection slice("LuaVM.Call");

	const auto L = function.L();

	LuaStackGuard stack_guard(L);
	PushCustomErrorHandler(L);

	const auto stack_top = lua_gettop(L);

	function.Push();
	for (auto &arg : args)
		arg.Push();

	ResetExecutionWatchdog(L);
	if (lua_pcall(L, numeric_cast<int>(args.size()), ret_vals != nullptr ? LUA_MULTRET : 0, -int(2 + args.size())) != LUA_OK) // perform call
		return false;

	if (ret_vals)
		GatherReturnValues(L, stack_top, *ret_vals);

	return true;
}

//
static int l_my_print(lua_State *L) {
	std::string msg;

	const auto nargs = lua_gettop(L);
	for (auto i = 1; i <= nargs; i++)
		if (lua_isstring(L, i))
			msg += lua_tostring(L, i);

	log(msg.c_str(), GetVMName(L).c_str());
	lua_pop(L, 1);
	return 0;
}

static const struct luaL_Reg custom_functions[] = {{"print", l_my_print}, {nullptr, nullptr}};

//
const char *GetLuaInterpreter() { return LUA_RELEASE; }

//
LuaObject CreateNil(lua_State *L) {
	lua_pushnil(L);
	return LuaObject(L, luaL_ref(L, LUA_REGISTRYINDEX)); // pop and store nil
}

LuaObject CreateEnv(lua_State *L, bool transfer_safe_global_symbols) {
	lua_newtable(L);

	lua_pushstring(L, LUA_RELEASE);
	lua_setfield(L, -2, "interpreter_name");

	lua_pushinteger(L, LUA_VERSION_NUM);
	lua_setfield(L, -2, "interpreter_version");

	if (transfer_safe_global_symbols) {
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "_G");

		static const char *symbols[] = {"error", "print", "require", "getmetatable", "setmetatable", "coroutine", "pairs", "ipairs", "table", "io", "os", "string",
			"tostring", "tonumber", "rawset", /*"bit32",*/ "math", "debug", "package", "type", /*"bit",*/ "assert", "load", "pcall", nullptr};

		lua_pushglobaltable(L);

		for (const char **symbol = symbols; *symbol; ++symbol) {
			lua_getfield(L, -1, *symbol);
			lua_setfield(L, -3, *symbol);
		}

		lua_pop(L, 1);
	}

	return LuaObject(L, luaL_ref(L, LUA_REGISTRYINDEX)); // pop and store table
}

LuaObject CreateTable(lua_State *L) {
	lua_newtable(L);
	return LuaObject(L, luaL_ref(L, LUA_REGISTRYINDEX)); // pop and store table
}

//
void SetMetatable(LuaObject &table, LuaObject &metatable) {
	table.Push();
	metatable.Push();
	lua_setmetatable(table.L(), -2);
	lua_pop(table.L(), 1);
}

//
lua_State *NewLuaVM() {
	const auto L = luaL_newstate();

	// setup libraries
	luaL_openlibs(L);

	// setup custom functions overrides
	lua_getglobal(L, "_G");
	luaL_setfuncs(L, custom_functions, 0);
	lua_pop(L, 1);
	return L;
}

void DestroyLuaVM(lua_State *L) { lua_close(L); }

} // namespace hg
