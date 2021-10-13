// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/time.h"

#include <string>
#include <vector>

struct lua_State;

namespace hg {

class LuaObject {
public:
	LuaObject();
	LuaObject(lua_State *L, int _ref = -2) : L_(L), ref(_ref) {} // LUA_NOREF -> -2
	LuaObject(const LuaObject &o);
	LuaObject(LuaObject &&o);
	~LuaObject();

	lua_State *L() const { return L_; }
	int Ref() const { return ref; }

	LuaObject &operator=(const LuaObject &o);
	LuaObject &operator=(LuaObject &&o);

	operator bool() const;

	void Push() const;
	void Clear();

private:
	lua_State *L_;
	int ref;
};

//
struct LuaStackGuard {
	LuaStackGuard(lua_State *L);
	~LuaStackGuard();

private:
	lua_State *L;
	int top;
};

//
struct LuaCallstack {
	struct Frame {
		struct Local {
			std::string name;
			LuaObject value;
		};

		std::string location, file;
		size_t line{0};

		std::vector<Local> locals;
	};

	std::vector<Frame> frames;
};

//
lua_State *NewLuaVM();
void DestroyLuaVM(lua_State *L);

std::string GetVMName(lua_State *L);
void SetVMName(lua_State *L, const std::string &name);

const char *GetLuaInterpreter();

//
LuaObject CreateNil(lua_State *L);
LuaObject CreateEnv(lua_State *L, bool transfer_safe_global_symbols);
LuaObject CreateTable(lua_State *L);

//
void SetMetatable(LuaObject &table, LuaObject &metatable);

//
void InstallExecutionWatchdog(lua_State *L, time_ns timeout);
void SetExecutionWatchdogTimeout(lua_State *L, time_ns timeout);
void ResetExecutionWatchdog(lua_State *L);
bool PingExecutionWatchdog(lua_State *L);

void PushCustomErrorHandler(lua_State *L);

//
LuaObject Pop(lua_State *L);

//
LuaObject MakeLuaObj(lua_State *L, bool v);
LuaObject MakeLuaObj(lua_State *L, int v);
LuaObject MakeLuaObj(lua_State *L, float v);
LuaObject MakeLuaObj(lua_State *L, const std::string &v);

bool LuaObjValue(const LuaObject &o, bool v);
int LuaObjValue(const LuaObject &o, int v);
float LuaObjValue(const LuaObject &o, float v);
std::string LuaObjValue(const LuaObject &o, std::string v);

//
LuaObject Get(lua_State *L, const std::string &key);
LuaObject Get(const LuaObject &table, const std::string &key);
LuaObject Get(const LuaObject &table, int key);

void Set(const std::string &key, const LuaObject &value);
void Set(const LuaObject &table, const std::string &key, const LuaObject &value);
void Set(const LuaObject &table, int key, const LuaObject &value);

//
bool Compile(lua_State *L, const std::string &source, const std::string &name, LuaObject &chunk_out);
bool Execute(lua_State *L, const std::string &source, const std::string &name, LuaObject *env = nullptr, std::vector<LuaObject> *ret_vals = nullptr);

bool Call(const LuaObject &function, const std::vector<LuaObject> &args = {}, std::vector<LuaObject> *ret_vals = nullptr);

void GatherReturnValues(lua_State *L, int start_index, std::vector<LuaObject> &ret_vals);

// Pop error on top of stack and send it to error().
void GetAndReportError(lua_State *L);

//
struct LuaVM {
	LuaVM() : L(NewLuaVM()) {}
	~LuaVM() { DestroyLuaVM(L); }

	operator lua_State *() const { return L; }

	lua_State *L;
};

} // namespace hg
