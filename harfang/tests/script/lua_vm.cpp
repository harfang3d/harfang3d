// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "script/lua_vm.h"

#include "foundation/log.h"

using namespace hg;

static void test_OpenCloseVM() { 
	LuaVM vm; 
}

static void test_ExecuteSource() {
	LuaVM vm;
	TEST_CHECK(Execute(vm, "io.write(\"Hello Lua!\")", "test") == true);
}

static void test_Get() {
	LuaVM vm;

	TEST_CHECK(Execute(vm, "global_value = 6", "test") == true);

	auto global_value_object = Get(vm, "global_value"); // grab variable object, no context is global context
	TEST_CHECK(global_value_object == true);

//	auto v = vm->ObjectToValue(global_value_object);

//	EXPECT_TRUE(v.GetType() == g_type_registry.get().GetType<int>());
//	EXPECT_EQ(6, v.Cast<int>());

	global_value_object.Clear();
}

static void test_ExecuteInvalidSource() {
	LuaVM vm;
	TEST_CHECK(Execute(vm, "non_existing_symbol()", "test") == false);
}

void test_lua_vm() { 
	test_OpenCloseVM();
	test_ExecuteSource();
	test_Get();
	test_ExecuteInvalidSource();
}