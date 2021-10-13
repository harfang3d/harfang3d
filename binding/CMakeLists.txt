add_custom_command(
	OUTPUT
		${CMAKE_BINARY_DIR}/binding/bind_Lua.cpp
		${CMAKE_BINARY_DIR}/binding/bind_Lua.h
		${CMAKE_BINARY_DIR}/binding/fabgen.h 
	COMMAND
		${Python3_EXECUTABLE} bind.py ${CMAKE_CURRENT_SOURCE_DIR}/bind_harfang.py --lua --embedded --prefix hg_lua --out ${CMAKE_BINARY_DIR}/binding ${HG_BINDING_DEFINES}
	MAIN_DEPENDENCY
		${CMAKE_CURRENT_SOURCE_DIR}/bind_harfang.py
	WORKING_DIRECTORY
		${HG_FABGEN_PATH}
	COMMENT
		"Generating Harfang binding for embedded Lua: python=${Python3_EXECUTABLE} fabgen=${HG_FABGEN_PATH} srcdir=${CMAKE_CURRENT_SOURCE_DIR} dstdir=${CMAKE_BINARY_DIR}")

# [EJ] Use an object library so that this fairly expensive source is only built
# once. A static libray won't work here since bind_Lua.cpp really is a part of
# the engine library.
add_library(
		bind_hg_lua
	OBJECT
		${CMAKE_BINARY_DIR}/binding/bind_Lua.cpp
		${CMAKE_BINARY_DIR}/binding/bind_Lua.h
		${CMAKE_BINARY_DIR}/binding/fabgen.h)

set_target_properties(bind_hg_lua PROPERTIES FOLDER "harfang")
target_link_libraries(bind_hg_lua jsonhpp libluadll imgui)
target_include_directories(bind_hg_lua 
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/binding>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../harfang>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../extern>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../extern/bgfx/bgfx/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../extern/bgfx/bimg/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../extern/bgfx/bx/include>
		$<INSTALL_INTERFACE:${CppSdkHeadersDestination}>
)
if(HG_ENABLE_RECAST_DETOUR_API)
	target_include_directories(bind_hg_lua 
		PUBLIC
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../extern/recastnavigation/DetourCrowd/Include>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../extern/recastnavigation/Detour/Include>
	)
endif()

set_target_properties(bind_hg_lua PROPERTIES PUBLIC_HEADER "${CMAKE_BINARY_DIR}/binding/bind_Lua.h;${CMAKE_BINARY_DIR}/binding/fabgen.h")

if(HG_BUILD_CPP_SDK)
	install(
		TARGETS bind_hg_lua
		EXPORT cppsdkTargets
		PUBLIC_HEADER DESTINATION cppsdk/include
		COMPONENT cppsdk
	)
endif()