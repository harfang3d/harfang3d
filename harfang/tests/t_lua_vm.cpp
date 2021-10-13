// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "script/lua_vm.h"
#include "foundation/log.h"
#include "shared.h"
#include "gtest/gtest.h"

using namespace hg;

TEST(LuaVM, OpenCloseVM) { LuaVM vm; }

TEST(LuaVM, ExecuteSource) {
	LuaVM vm;
	Execute(vm, "io.write(\"Hello Lua!\")", "test");
}

TEST(LuaVM, Get) {
	LuaVM vm;

	EXPECT_TRUE(Execute(vm, "global_value = 6", "test"));

	auto global_value_object = Get(vm, "global_value"); // grab variable object, no context is global context
	EXPECT_TRUE(global_value_object);

//	auto v = vm->ObjectToValue(global_value_object);

//	EXPECT_TRUE(v.GetType() == g_type_registry.get().GetType<int>());
//	EXPECT_EQ(6, v.Cast<int>());

	global_value_object.Clear();
}

TEST(LuaVM, ExecuteInvalidSource) {
	LuaVM vm;
	Execute(vm, "non_existing_symbol()", "test");
}

#if 0

TEST(LuaVM, Set) {
	auto vm = std::make_shared<LuaVM>("VM");
	EXPECT_TRUE(vm->Open());

	auto object = vm->CreateObject(6);
	EXPECT_TRUE(vm->Set("base", object));

	EXPECT_TRUE(vm->Execute("global_value = base * 3", "test"));

	object = vm->Get("global_value"); // grab variable object, no context is global context

	auto r = vm->ObjectToValue(object);
	EXPECT_TRUE(r.GetType() == g_type_registry.get().GetType<int>());
	EXPECT_EQ(18, r.Cast<int>());

	object.Release();
	vm->Close();
}

TEST(LuaVM, SetInContext) {
	auto vm = std::make_shared<LuaVM>("VM");
	EXPECT_TRUE(vm->Open());
	EXPECT_TRUE(vm->Execute("Lib = {}", "test"));

	auto val = vm->CreateObject(8);
	auto lib = vm->Get("Lib"); // grab Lib object
	vm->Set("value", val, lib);
	lib.Release();

	EXPECT_TRUE(vm->Execute("global_value = Lib.value * 3", "test"));

	val = vm->Get("global_value"); // grab variable object, no context is global context

	auto r = vm->ObjectToValue(val);
	val.Release();

	EXPECT_TRUE(r.GetType() == g_type_registry.get().GetType<int>());
	EXPECT_EQ(24, r.Cast<int>());
	vm->Close();
}

TEST(LuaVM, CallGlobalFunction) {
	auto vm = std::make_shared<LuaVM>("VM");
	EXPECT_TRUE(vm->Open());

	std::string source =
		"function fn()\n\
	io.write(\"Function called.\");\n\
end";
	EXPECT_TRUE(vm->Execute(source, "test"));

	auto fn = vm->Get("fn"); // grab function object, no context is global context

	EXPECT_TRUE(vm->Call(fn));

	fn.Release(); // throw object away before the VM is closed
	vm->Close();
}

TEST(LuaVM, CallNonGlobalFunction) {
	auto vm = std::make_shared<LuaVM>("VM");
	EXPECT_TRUE(vm->Open());

	std::string source =
		"Lib = {}\n\
Lib.fn = function ()\n\
	io.write(\"Library function called.\");\n\
end";
	EXPECT_TRUE(vm->Execute(source, "test"));

	auto lib = vm->Get("Lib"); // grab Lib object
	auto fn = vm->Get("fn", lib); // grab function object in Lib context

	EXPECT_TRUE(vm->Call(fn));

	fn.Release(); // throw objects away before the VM is closed
	lib.Release();
	vm->Close();
}

TEST(LuaVM, CallFunctionWithArgs) {
	auto vm = std::make_shared<LuaVM>("VM");
	EXPECT_TRUE(vm->Open());

	std::string source =
		"function fn(arg0, arg1)\n\
	io.write(\"Function called with arguments: \" .. arg0 .. \", \" .. arg1);\n\
end";
	EXPECT_TRUE(vm->Execute(source, "test"));

	auto fn = vm->Get("fn");
	{
		std::vector<ScriptObject> args = {
			vm->CreateObject(8),
			vm->CreateObject(std::string("I am a string!"))};
		EXPECT_TRUE(vm->Call(fn, args));
	}

	fn.Release(); // throw objects away before the VM is closed
	vm->Close();
}

TEST(LuaVM, ReturnValueToHost) {
	auto vm = std::make_shared<LuaVM>("VM");
	EXPECT_TRUE(vm->Open());

	std::string source =
		"function fn()\n\
	return \"Another native string...\";\n\
end";
	EXPECT_TRUE(vm->Execute(source, "test"));

	auto fn = vm->Get("fn"); // grab function object, no context is global context

	std::vector<ScriptObject> ret;
	EXPECT_TRUE(vm->Call(fn, {}, &ret));

	EXPECT_EQ(1, ret.size());
	TypeValue out = vm->ObjectToValue(ret[0]);

	EXPECT_TRUE(out.GetType() == g_type_registry.get().GetType<std::string>());
	EXPECT_TRUE(out.Cast<std::string>() == "Another native string...");

	ret[0].Release();
	fn.Release();
	vm->Close();
}

TEST(LuaVM, DoFileFromMountPoint) {
	auto vm = std::make_shared<LuaVM>("VM");

	EXPECT_TRUE(vm->Open());

	EXPECT_FALSE(vm->Execute("dofile(\"@missing/src.lua\")", "test"));

	g_fs.get().Mount(std::make_shared<StdFileDriver>(GetResPath("lua_scripts").c_str()), "@scripts");
	EXPECT_TRUE(vm->Execute("dofile(\"@scripts/dummy_script.lua\")", "test"));

	{
		auto res = vm->Get("dummy_script_created");

		TypeValue out = vm->ObjectToValue(res);

		EXPECT_TRUE(out.GetType() == g_type_registry.get().GetType<std::string>());
		EXPECT_TRUE(out.Cast<std::string>() == "OK");
	}

	EXPECT_FALSE(vm->Execute("dofile(\"@scripts/invalid_script.lua\")", "test"));

	vm->Close();
}

static int on_runtime_error = 0;
static void OnRuntimeError(const ScriptRuntimeError &) { ++on_runtime_error; }

TEST(LuaVM, RuntimeErrorEventHook) {
	auto vm = std::make_shared<LuaVM>("VM");
	vm->runtime_error_signal.Connect(&OnRuntimeError);

	EXPECT_TRUE(vm->Open());
	EXPECT_FALSE(vm->Execute("nil_function_call()", "test"));
	EXPECT_GT(on_runtime_error, 0);

	vm->Close();
}

TEST(LuaVM, CompileChunkAndCall) {
	auto vm = std::make_shared<LuaVM>("VM");

	EXPECT_TRUE(vm->Open());
	ScriptObject chunk;
	EXPECT_TRUE(vm->Compile(
		""
		"local a1, a2 = ...\n"
		""
		""
		"print('Chunk called with a1 = '..tostring(a1)..' and a2 = '..tostring(a2))"
		"",
		"test",
		chunk));
	EXPECT_TRUE(chunk.IsValidAndNonNull());

	std::vector<ScriptObject> args = {
		vm->CreateObject(8),
		vm->CreateObject(std::string("I am a string!"))};
	EXPECT_TRUE(vm->Call(chunk, args));

	vm->Close();
}

TEST(LuaVM, CompileChunkAndCallWithReturnValues) {
	auto vm = std::make_shared<LuaVM>("VM");

	EXPECT_TRUE(vm->Open());
	ScriptObject chunk;
	EXPECT_TRUE(vm->Compile(
		""
		"local a1, a2 = ...\n"
		""
		""
		"return a1 * 4, a2..' And I was modified by Lua!'"
		"",
		"test",
		chunk));
	EXPECT_TRUE(chunk.IsValidAndNonNull());

	std::vector<ScriptObject> args = {
		vm->CreateObject(8),
		vm->CreateObject(std::string("I am a string!"))};

	std::vector<ScriptObject> rvals;
	EXPECT_TRUE(vm->Call(chunk, args, &rvals));

	EXPECT_EQ(2, rvals.size());

	int rval0 = vm->CastObject<int>(rvals[0], -1);
	std::string rval1 = vm->CastObject<std::string>(rvals[1], "empty");

	EXPECT_EQ(32, rval0);
	EXPECT_EQ(std::string("I am a string! And I was modified by Lua!"), rval1);

	rvals[0].Release();
	rvals[1].Release();

	vm->Close();
}

TEST(LuaVM, CompileChunkAndCallMultipleTimesWithReturnValues) {
	auto vm = std::make_shared<LuaVM>("VM");

	EXPECT_TRUE(vm->Open());
	ScriptObject chunk;
	EXPECT_TRUE(vm->Compile(
		""
		"local a1, a2 = ...\n"
		""
		""
		"return a1 * 4, a2..' And I was modified by Lua!'"
		"",
		"test",
		chunk));
	EXPECT_TRUE(chunk.IsValidAndNonNull());

	{
		std::vector<ScriptObject> args = {
			vm->CreateObject(8),
			vm->CreateObject(std::string("I am a string!"))};

		std::vector<ScriptObject> rvals;
		EXPECT_TRUE(vm->Call(chunk, args, &rvals));

		EXPECT_EQ(2, rvals.size());

		int rval0 = vm->CastObject<int>(rvals[0], -1);
		std::string rval1 = vm->CastObject<std::string>(rvals[1], "empty");

		EXPECT_EQ(32, rval0);
		EXPECT_EQ(std::string("I am a string! And I was modified by Lua!"), rval1);

		rvals[0].Release();
		rvals[1].Release();
	}

	{
		std::vector<ScriptObject> args = {
			vm->CreateObject(7),
			vm->CreateObject(std::string("I am another string!"))};

		std::vector<ScriptObject> rvals;
		EXPECT_TRUE(vm->Call(chunk, args, &rvals));

		EXPECT_EQ(2, rvals.size());

		int rval0 = vm->CastObject<int>(rvals[0], -1);
		std::string rval1 = vm->CastObject<std::string>(rvals[1], "empty");

		EXPECT_EQ(28, rval0);
		EXPECT_EQ(std::string("I am another string! And I was modified by Lua!"), rval1);

		rvals[0].Release();
		rvals[1].Release();
	}

	vm->Close();
}

#endif
