// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "fabgen.h"

#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/log.h"

#include "engine/assets_rw_interface.h"
#include "engine/physics.h"
#include "engine/scene.h"
#include "engine/scene_lua_vm.h"

#include "bind_Lua.h"

extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

#include <set>

namespace hg {

ScriptParam ScriptParamFromLuaObject(const LuaObject &v) {
	ScriptParam parm{SPT_Null};

	if (v) {
		v.Push();
		auto L = v.L();

		if (lua_isboolean(L, -1)) {
			parm = {SPT_Bool};
			parm.bv = lua_toboolean(L, -1);
		} else if (lua_isinteger(L, -1)) {
			parm = {SPT_Int};
			parm.iv = numeric_cast<int>(lua_tointeger(L, -1));
		} else if (lua_isnumber(L, -1)) {
			parm = {SPT_Float};
			parm.fv = float(lua_tonumber(L, -1));
		} else if (lua_isstring(L, -1)) {
			parm = {SPT_String};
			parm.sv = lua_tostring(L, -1);
		}

		lua_pop(L, 1);
	}

	return parm;
}

LuaObject LuaObjectFromScriptParam(lua_State *L, const ScriptParam &p) {
	if (p.type == SPT_Bool)
		lua_pushboolean(L, p.bv);
	else if (p.type == SPT_Int)
		lua_pushinteger(L, p.iv);
	else if (p.type == SPT_Float)
		lua_pushnumber(L, p.fv);
	else if (p.type == SPT_String)
		lua_pushstring(L, p.sv.c_str());
	else
		lua_pushnil(L);

	return Pop(L);
}

//
bool SceneLuaVM::CreateScriptFromSource(Scene &scene, ComponentRef ref, const std::string &src) {
	const auto i = lua_scripts.find(ref);

	if (i != std::end(lua_scripts))
		return false; // already in scene

	const auto path = scene.GetScriptPath(ref);

	if (!src.empty()) {
		auto &env = lua_scripts[ref]; // create new env
		env = CreateEnv(L, true);

		Set(env, "G", G);
		Set(env, "hg", hg);

		if (Execute(L, src, path, &env))
			for (auto i : scene.GetScriptParams(ref))
				SetScriptValue(ref, i.first, LuaObjectFromScriptParam(L, i.second));
	} else {
		warn(format("Failed to load Lua file '%1'").arg(path));
		return false;
	}

	return true;
}

bool SceneLuaVM::CreateScript(Scene &scene, ComponentRef ref, const Reader &ir, const ReadProvider &ip) {
	const auto path = scene.GetScriptPath(ref);

	std::string src;
	const auto j = src_overrides.find(ref);

	if (j == std::end(src_overrides)) {
		ScopedReadHandle h(ip, path.c_str());
		src = LoadString(ir, h);
	} else {
		src = j->second;
		src_overrides.erase(j);
	}

	return CreateScriptFromSource(scene, ref, src);
}

bool SceneLuaVM::CreateScriptFromFile(Scene &scene, ComponentRef ref) { return CreateScript(scene, ref, g_file_reader, g_file_read_provider); }
bool SceneLuaVM::CreateScriptFromAssets(Scene &scene, ComponentRef ref) { return CreateScript(scene, ref, g_assets_reader, g_assets_read_provider); }

//
std::vector<ComponentRef> SceneLuaVM::CreateNodeScripts(Scene &scene, NodeRef ref, const Reader &ir, const ReadProvider &ip) {
	std::vector<ComponentRef> components;

	for (size_t i = 0; i < scene.GetNodeScriptCount(ref); ++i) {
		auto script = scene.GetNodeScript(ref, i);

		if (scene.IsValidScriptRef(script.ref))
			if (CreateScript(scene, script.ref, ir, ip))
				components.push_back(script.ref);
	}

	return components;
}

std::vector<ComponentRef> SceneLuaVM::CreateNodeScriptsFromFile(Scene &scene, NodeRef ref) {
	return CreateNodeScripts(scene, ref, g_file_reader, g_file_read_provider);
}

std::vector<ComponentRef> SceneLuaVM::CreateNodeScriptsFromAssets(Scene &scene, NodeRef ref) {
	return CreateNodeScripts(scene, ref, g_assets_reader, g_assets_read_provider);
}

std::vector<Script> SceneLuaVM::CreateNodeScriptsFromFile(Scene &scene, const Node &node) {
	std::vector<Script> out;

	for (auto script : CreateNodeScriptsFromFile(scene, node.ref))
		out.push_back(scene.GetScript(script));

	return out;
}

std::vector<Script> SceneLuaVM::CreateNodeScriptsFromAssets(Scene &scene, const Node &node) {
	std::vector<Script> out;

	for (auto script : CreateNodeScriptsFromAssets(scene, node.ref))
		out.push_back(scene.GetScript(script));

	return out;
}

//
std::vector<ComponentRef> SceneLuaVM::SceneCreateScripts(Scene &scene, const Reader &ir, const ReadProvider &ip) {
	std::vector<ComponentRef> created_scripts;

	for (auto ref : scene.GetScriptRefs())
		if (CreateScript(scene, ref, ir, ip))
			created_scripts.push_back(ref);

	return created_scripts;
}

std::vector<ComponentRef> SceneLuaVM::SceneCreateScriptsFromFile(Scene &scene) { return SceneCreateScripts(scene, g_file_reader, g_file_read_provider); }
std::vector<ComponentRef> SceneLuaVM::SceneCreateScriptsFromAssets(Scene &scene) { return SceneCreateScripts(scene, g_assets_reader, g_assets_read_provider); }

//
LuaObject SceneLuaVM::GetScriptEnv(ComponentRef ref) const {
	auto i = lua_scripts.find(ref);
	return i != std::end(lua_scripts) ? i->second : LuaObject{};
}

//
LuaObject SceneLuaVM::GetScriptValue(ComponentRef ref, const std::string &name) const {
	auto i = lua_scripts.find(ref);
	return i == std::end(lua_scripts) ? LuaObject{} : Get(i->second, name);
}

bool SceneLuaVM::SetScriptValue(ComponentRef ref, const std::string &name, const LuaObject &v) {
	auto i = lua_scripts.find(ref);

	if (i == std::end(lua_scripts))
		return false;

	SetForeign(i->second, name, v);
	return true;
}

//
bool SceneLuaVM::Call(const LuaObject &function, const std::vector<LuaObject> &args, std::vector<LuaObject> *ret_vals) {
	const auto tgt_L = function.L();

	// transfer any foreign argument to the target VM (the function VM)
	std::vector<LuaObject> L_args;
	L_args.reserve(args.size());

	for (auto &arg : args)
		if (arg.L() != tgt_L) {
			PushForeign(tgt_L, arg);
			L_args.push_back(Pop(tgt_L));
		} else {
			L_args.push_back(arg);
		}

	// execute call
	return hg::Call(function, L_args, ret_vals);
}

bool SceneLuaVM::Call(ComponentRef ref, const std::string &function, const std::vector<LuaObject> &args, std::vector<LuaObject> *ret_vals) {
	auto i = lua_scripts.find(ref);

	if (i == std::end(lua_scripts))
		return false;

	if (auto fn = Get(i->second, function))
		return Call(fn, args, ret_vals);

	return false;
}

//
void SceneLuaVM::ForeachScripts(const std::function<void(const LuaObject &)> &cb) const {
	for (auto i : lua_scripts)
		cb(i.second);
}

void SceneLuaVM::ForeachScriptsInSceneContext(
	Scene &scene, const std::vector<ComponentRef> &scripts, const std::function<void(Scene &, const LuaObject &)> &cb) const {
	std::set<ComponentRef> scene_scripts;

	for (auto i : scene.GetSceneScripts())
		scene_scripts.insert(i);

	for (auto script : scripts) {
		auto i = scene_scripts.find(script);

		if (i != std::end(scene_scripts)) {
			auto j = lua_scripts.find(script);
			if (j != std::end(lua_scripts))
				cb(scene, j->second);
		}
	}
}

void SceneLuaVM::ForeachScriptsInNodeContext(
	const Scene &scene, const std::vector<ComponentRef> &scripts, const std::function<void(const Scene &, Node &, const LuaObject &)> &cb) const {
	std::map<ComponentRef, std::vector<NodeRef>> script_to_node;

	for (auto i : scene.GetNodeScripts())
		for (auto j : i.second)
			script_to_node[j].push_back(i.first);

	for (auto script : scripts) {
		auto i = script_to_node.find(script);

		if (i != std::end(script_to_node))
			for (auto node_ref : i->second) {
				auto node = scene.GetNode(node_ref);

				auto j = lua_scripts.find(script);
				if (j != std::end(lua_scripts))
					cb(scene, node, j->second);
			}
	}
}

//
void SceneLuaVM::ForeachSceneScripts(Scene &scene, const std::function<void(Scene &, const LuaObject &)> &cb) const {
	for (auto cref : scene.GetSceneScripts()) {
		auto i = lua_scripts.find(cref);
		if (i != std::end(lua_scripts))
			cb(scene, i->second);
	}
}

void SceneLuaVM::ForeachNodeScripts(const Scene &scene, NodeRef node_ref, const std::function<void(const Scene &, Node &, const LuaObject &)> &cb) const {
	auto node = scene.GetNode(node_ref);
	auto &node_scripts = scene.GetNodeScripts();

	auto i = node_scripts.find(node_ref);
	if (i != std::end(node_scripts))
		for (auto cref : i->second) {
			auto j = lua_scripts.find(cref);
			if (j != std::end(lua_scripts))
				cb(scene, node, j->second);
		}
}

void SceneLuaVM::ForeachAllNodesScripts(const Scene &scene, const std::function<void(const Scene &, Node &, const LuaObject &)> &cb) const {
	for (auto i : scene.GetNodeScripts()) {
		auto node = scene.GetNode(i.first);

		for (auto cref : i.second) {
			auto j = lua_scripts.find(cref);
			if (j != std::end(lua_scripts))
				cb(scene, node, j->second);
		}
	}
}

//
std::vector<ComponentRef> SceneLuaVM::GarbageCollect(const Scene &scene) const {
	std::vector<ComponentRef> garbage;
	for (auto i : lua_scripts)
		if (!scene.IsValidScriptRef(i.first)) // component not in scene anymore
			garbage.push_back(i.first);
	return garbage;
}

void SceneLuaVM::DestroyScripts(const std::vector<ComponentRef> &scripts) {
	for (auto ref : scripts) {
		auto i = lua_scripts.find(ref);
		if (i != std::end(lua_scripts))
			lua_scripts.erase(i);
	}
}

//
std::vector<std::string> SceneLuaVM::GetScriptInterface(ComponentRef ref) const {
	std::vector<std::string> vars;

	auto i = lua_scripts.find(ref);
	if (i != std::end(lua_scripts)) {
		const auto itf = Get(i->second, "interface");

		if (itf) {
			itf.Push();
			lua_len(L, -1);

			auto isnum = 0;
			const auto len = lua_tointegerx(L, -1, &isnum);

			if (isnum) {
				for (auto i = 1; i <= len; ++i) {
					const auto type = lua_geti(L, -2, i);

					if (type == LUA_TSTRING)
						vars.emplace_back(lua_tostring(L, -1));

					lua_pop(L, 1);
				}
			}

			lua_pop(L, 2);
		}
	}
	return vars;
}

//
void SceneLuaVM::Clear() {
	lua_scripts.clear();
	src_overrides.clear();
}

//
SceneLuaVM::SceneLuaVM() {
	L = NewLuaVM();
	SetVMName(L, "SceneVM");

	G = CreateEnv(L, false); // empty shared environment
	Set("G", G);

	hg_lua_bind_harfang(L, "hg");
	hg = Get(L, "hg");
}

SceneLuaVM::~SceneLuaVM() {
	lua_scripts.clear();

	G.Clear();
	hg.Clear();

	DestroyLuaVM(L);
}

} // namespace hg
