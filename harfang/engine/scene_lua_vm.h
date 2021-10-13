// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/lua_object.h"
#include "engine/node.h"
#include "engine/script_param.h"

#include "foundation/generational_vector_list.h"
#include "foundation/rw_interface.h"

#include "script/lua_vm.h"

#include <functional>
#include <map>

namespace hg {

class Scene;

/// Provide Lua scripting capabilities to a Scene
class SceneLuaVM {
public:
	SceneLuaVM();
	~SceneLuaVM();

	/// Create a single script component.
	bool CreateScriptFromSource(Scene &scene, ComponentRef ref, const std::string &src);

	bool CreateScript(Scene &scene, ComponentRef ref, const Reader &ir, const ReadProvider &ip);
	bool CreateScriptFromFile(Scene &scene, ComponentRef ref);
	bool CreateScriptFromAssets(Scene &scene, ComponentRef ref);

	bool CreateScriptFromSource(Scene &scene, const Script &script, const std::string &src) { return CreateScriptFromSource(scene, script.ref, src); }
	bool CreateScriptFromFile(Scene &scene, const Script &script) { return CreateScriptFromFile(scene, script.ref); }
	bool CreateScriptFromAssets(Scene &scene, const Script &script) { return CreateScriptFromAssets(scene, script.ref); }

	/// Create all script components attached to a node.
	std::vector<ComponentRef> CreateNodeScripts(Scene &scene, NodeRef ref, const Reader &ir, const ReadProvider &ip);
	std::vector<ComponentRef> CreateNodeScriptsFromFile(Scene &scene, NodeRef ref);
	std::vector<ComponentRef> CreateNodeScriptsFromAssets(Scene &scene, NodeRef ref);

	std::vector<Script> CreateNodeScriptsFromFile(Scene &scene, const Node &node);
	std::vector<Script> CreateNodeScriptsFromAssets(Scene &scene, const Node &node);

	/// Create all script components in a scene, already existing components are ignored.
	std::vector<ComponentRef> SceneCreateScripts(Scene &scene, const Reader &ir, const ReadProvider &ip);
	std::vector<ComponentRef> SceneCreateScriptsFromFile(Scene &scene);
	std::vector<ComponentRef> SceneCreateScriptsFromAssets(Scene &scene);

	/// Return all scripts that are not found in scene anymore.
	std::vector<ComponentRef> GarbageCollect(const Scene &scene) const;
	/// Destroy the provided scripts.
	void DestroyScripts(const std::vector<ComponentRef> &scripts);

	/// Return the number of script component setup on this VM.
	size_t GetScriptCount() const { return lua_scripts.size(); }

	/// Return a script execution environment object.
	LuaObject GetScriptEnv(ComponentRef script) const;

	/// Return a new empty lua object.
	LuaObject MakeLuaObject() const { return LuaObject(L); }

	/// Return the value of a script variable.
	LuaObject GetScriptValue(ComponentRef script, const std::string &name) const;
	/// Set the value of a script variable.
	bool SetScriptValue(ComponentRef script, const std::string &name, const LuaObject &value);

	/// Return the VM lua state.
	lua_State *GetL() const { return L; }
	/// Return the VM global table object.
	const LuaObject &GetG() const { return G; }

	/// Get a value from the VM global table (G).
	LuaObject GetGlobal(const std::string &key) { return Get(G, key); }
	/// Set a value in the VM global table (G).
	void SetGlobal(const std::string &key, const LuaObject &val) { SetForeign(G, key, val); }

	/// Call a function object on its VM, arguments can be passed from a different VM.
	bool Call(const LuaObject &function, const std::vector<LuaObject> &args, std::vector<LuaObject> *ret_vals = nullptr);
	/// Call a script function on its VM, arguments can be passed from a different VM.
	bool Call(ComponentRef ref, const std::string &function, const std::vector<LuaObject> &args, std::vector<LuaObject> *ret_vals = nullptr);

	bool Call(const Script &script, const std::string &function, const std::vector<LuaObject> &args, std::vector<LuaObject> *ret_vals) {
		return Call(script.ref, function, args, ret_vals);
	}

	/// For each script.
	void ForeachScripts(const std::function<void(const LuaObject &)> &cb) const;

	/// For each script callback in the context of the scene referencing it.
	void ForeachScriptsInSceneContext(Scene &scene, const std::vector<ComponentRef> &scripts, const std::function<void(Scene &, const LuaObject &)> &cb) const;
	/// For each script callback in the context of the node referencing it.
	void ForeachScriptsInNodeContext(
		const Scene &scene, const std::vector<ComponentRef> &scripts, const std::function<void(const Scene &, Node &, const LuaObject &)> &cb) const;

	/// For each script associated to a Scene.
	void ForeachSceneScripts(Scene &scene, const std::function<void(Scene &, const LuaObject &)> &cb) const;
	/// For each script associated to a Node.
	void ForeachNodeScripts(const Scene &scene, NodeRef node_ref, const std::function<void(const Scene &, Node &, const LuaObject &)> &cb) const;
	/// For each script associated to each Node of a scene.
	void ForeachAllNodesScripts(const Scene &scene, const std::function<void(const Scene &, Node &, const LuaObject &)> &cb) const;

	/// Return a list of variable names forming the script interface.
	std::vector<std::string> GetScriptInterface(ComponentRef ref) const;
	std::vector<std::string> GetScriptInterface(const Script &script) const { return GetScriptInterface(script.ref); }

	/// Override the source code for a specific script component. when created the provided source will be used regardless of the script path parameter.
	void OverrideScriptSource(ComponentRef ref, std::string src) { src_overrides[ref] = std::move(src); }

	/// Clear all scripts.
	void Clear();

private:
	lua_State *L;
	LuaObject G, hg;

	std::map<ComponentRef, LuaObject> lua_scripts;
	std::map<ComponentRef, std::string> src_overrides;
};

/// Create a script parameter from Lua object.
ScriptParam ScriptParamFromLuaObject(const LuaObject &v);
/// Return a script parameter value as Lua object.
LuaObject LuaObjectFromScriptParam(lua_State *L, const ScriptParam &p);

} // namespace hg
